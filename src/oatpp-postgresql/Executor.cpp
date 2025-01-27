/***************************************************************************
 *
 * Project         _____    __   ____   _      _
 *                (  _  )  /__\ (_  _)_| |_  _| |_
 *                 )(_)(  /(__)\  )( (_   _)(_   _)
 *                (_____)(__)(__)(__)  |_|    |_|
 *
 *
 * Copyright 2018-present, Leonid Stryzhevskyi <lganzzzo@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ***************************************************************************/

#include "Executor.hpp"

#include "ql_template/Parser.hpp"
#include "ql_template/TemplateValueProvider.hpp"

#include "QueryResult.hpp"

#include "oatpp/orm/Transaction.hpp"

#include "oatpp/core/data/stream/ChunkedBuffer.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/utils/ConversionUtils.hpp"

#include <vector>

namespace oatpp { namespace postgresql {

namespace {

  #include OATPP_CODEGEN_BEGIN(DTO)

  class VersionRow : public oatpp::DTO {

    DTO_INIT(VersionRow, DTO);

    DTO_FIELD(Int64, version);

  };

  #include OATPP_CODEGEN_END(DTO)

}

Executor::QueryParams::QueryParams(const StringTemplate& queryTemplate,
                                   const std::unordered_map<oatpp::String, oatpp::Void>& params,
                                   const mapping::Serializer& serializer,
                                   const std::shared_ptr<const data::mapping::TypeResolver>& typeResolver)
{

  data::mapping::TypeResolver::Cache cache;

  auto extra = std::static_pointer_cast<ql_template::Parser::TemplateExtra>(queryTemplate.getExtraData());

  query = extra->preparedTemplate->c_str();
  if(extra->templateName) {
    queryName = extra->templateName->c_str();
  } else {
    queryName = nullptr;
  }

  count = queryTemplate.getTemplateVariables().size();

  outData.resize(count);
  paramOids.resize(count);
  paramValues.resize(count);
  paramLengths.resize(count);
  paramFormats.resize(count);

  for(v_uint32 i = 0; i < count; i ++) {

    const auto& var = queryTemplate.getTemplateVariables()[i];
    auto it = params.find(var.name);

    auto queryParameter = parseQueryParameter(var.name);
    if(queryParameter.name) {

      it = params.find(queryParameter.name);
      if(it != params.end()) {

        auto value = typeResolver->resolveObjectPropertyValue(it->second, queryParameter.propertyPath, cache);
        if(value.valueType->classId.id == oatpp::Void::Class::CLASS_ID.id) {
          std::string tname = "UnNamed";
          if(extra->templateName) {
            tname = extra->templateName->std_str();
          }
          throw std::runtime_error("[oatpp::postgresql::Executor::QueryParams::QueryParams()]: "
                                   "Error."
                                   " Query '" + tname +
                                   "', parameter '" + var.name->std_str() +
                                   "' - property not found or its type is unknown.");
        }

        auto& data = outData[i];
        serializer.serialize(data, value);

        paramOids[i] = data.oid;
        paramValues[i] = data.data;
        paramLengths[i] = data.dataSize;
        paramFormats[i] = data.dataFormat;

        continue;

      }
    }

    throw std::runtime_error("[oatpp::postgresql::Executor::QueryParams::QueryParams()]: "
                             "Error. Parameter not found " + var.name->std_str());

  }

}

Executor::Executor(const std::shared_ptr<provider::Provider<Connection>>& connectionProvider)
  : m_connectionProvider(connectionProvider)
  , m_resultMapper(std::make_shared<mapping::ResultMapper>())
{
  m_defaultTypeResolver->addKnownClasses({
    Uuid::Class::CLASS_ID
  });
}

std::shared_ptr<data::mapping::TypeResolver> Executor::createTypeResolver() {
  auto typeResolver = std::make_shared<data::mapping::TypeResolver>();
  typeResolver->addKnownClasses({
    Uuid::Class::CLASS_ID
  });
  return typeResolver;
}

Executor::QueryParameter Executor::parseQueryParameter(const oatpp::String& paramName) {

  parser::Caret caret(paramName);
  auto nameLabel = caret.putLabel();
  if(caret.findChar('.') && caret.getPosition() < caret.getDataSize() - 1) {

    QueryParameter result;
    result.name = nameLabel.toString();

    do {

      caret.inc();
      auto label = caret.putLabel();
      caret.findChar('.');
      result.propertyPath.push_back(label.std_str());

    } while (caret.getPosition() < caret.getDataSize());

    return result;

  }

  return {nameLabel.toString(), {}};

}

std::unique_ptr<Oid[]> Executor::getParamTypes(const StringTemplate& queryTemplate,
                                               const ParamsTypeMap& paramsTypeMap,
                                               const std::shared_ptr<const data::mapping::TypeResolver>& typeResolver) {

  data::mapping::TypeResolver::Cache cache;

  std::unique_ptr<Oid[]> result(new Oid[queryTemplate.getTemplateVariables().size()]);

  for(v_uint32 i = 0; i < queryTemplate.getTemplateVariables().size(); i++) {

    const auto& var = queryTemplate.getTemplateVariables()[i];
    auto it = paramsTypeMap.find(var.name);

    auto queryParameter = parseQueryParameter(var.name);
    if(queryParameter.name) {

      it = paramsTypeMap.find(queryParameter.name);
      if(it != paramsTypeMap.end()) {
        auto type = typeResolver->resolveObjectPropertyType(it->second, queryParameter.propertyPath, cache);
        if(type) {
          result.get()[i] = m_serializer.getTypeOid(type);
          continue;
        }
      }

    }

    throw std::runtime_error("[oatpp::postgresql::Executor::getParamTypes()]: Error. "
                             "Type info not found for variable " + var.name->std_str());

  }

  return result;

}

std::shared_ptr<QueryResult> Executor::prepareQuery(const StringTemplate& queryTemplate,
                                                    const std::shared_ptr<const data::mapping::TypeResolver>& typeResolver,
                                                    const std::shared_ptr<postgresql::Connection>& connection)
{

  auto extra = std::static_pointer_cast<ql_template::Parser::TemplateExtra>(queryTemplate.getExtraData());
  auto paramTypes = getParamTypes(queryTemplate, extra->paramsTypeMap, typeResolver);

  PGresult *qres = PQprepare(connection->getHandle(),
                             extra->templateName->c_str(),
                             extra->preparedTemplate->c_str(),
                             queryTemplate.getTemplateVariables().size(),
                             paramTypes.get());

  return std::make_shared<QueryResult>(qres, connection, m_connectionProvider, m_resultMapper, typeResolver);

}

std::shared_ptr<QueryResult> Executor::executeQueryPrepared(const StringTemplate& queryTemplate,
                                                            const std::unordered_map<oatpp::String, oatpp::Void>& params,
                                                            const std::shared_ptr<const data::mapping::TypeResolver>& typeResolver,
                                                            const std::shared_ptr<postgresql::Connection>& connection)
{

  QueryParams queryParams(queryTemplate, params, m_serializer, typeResolver);

  PGresult *qres = PQexecPrepared(connection->getHandle(),
                                  queryParams.queryName,
                                  queryParams.count,
                                  queryParams.paramValues.data(),
                                  queryParams.paramLengths.data(),
                                  queryParams.paramFormats.data(),
                                  1);

  return std::make_shared<QueryResult>(qres, connection, m_connectionProvider, m_resultMapper, typeResolver);

}

std::shared_ptr<QueryResult> Executor::executeQuery(const StringTemplate& queryTemplate,
                                                    const std::unordered_map<oatpp::String, oatpp::Void>& params,
                                                    const std::shared_ptr<const data::mapping::TypeResolver>& typeResolver,
                                                    const std::shared_ptr<postgresql::Connection>& connection)
{

  QueryParams queryParams(queryTemplate, params, m_serializer, typeResolver);

  PGresult *qres = PQexecParams(connection->getHandle(),
                                queryParams.query,
                                queryParams.count,
                                queryParams.paramOids.data(),
                                queryParams.paramValues.data(),
                                queryParams.paramLengths.data(),
                                queryParams.paramFormats.data(),
                                1);

  return std::make_shared<QueryResult>(qres, connection, m_connectionProvider, m_resultMapper, typeResolver);

}

template<class ContainerPtr>
std::string expandQuery(ContainerPtr container, 
    const std::shared_ptr<ql_template::Parser::ArrayVariableExtra>& a_extra,
    std::unordered_map<String,Void>& params,
    orm::Executor::ParamsTypeMap& paramsTypeMap)
{
  std::string values;
  bool is_first = true;
  int index = 0;
  for(const Void& entity : *container)
  {
    values += std::string(is_first ? "" : ",") + "(";
    is_first = true;
    String entity_name = String("array_value") + utils::conversion::int32ToStr(index++);
    if(a_extra->variables.empty())
    {
      values += std::string(is_first ? "" : ",") + ":" + entity_name->c_str();
      is_first = false;
    }
    else{
      for(const auto& member : a_extra->variables)
      {
        values += std::string(is_first ? "" : ",") + ":" + entity_name->c_str() + "." + member.name->c_str();
        is_first = false;
      }
    }
    params.insert({entity_name, entity});
    paramsTypeMap.insert({entity_name, entity.valueType});
    values += ")";
  }
  return values;
}

std::shared_ptr<QueryResult> Executor::expandAndExecuteQuery(const StringTemplate& queryTemplate,
  const std::unordered_map<oatpp::String, oatpp::Void>& params,
  const std::shared_ptr<const data::mapping::TypeResolver>& typeResolver,
  const std::shared_ptr<postgresql::Connection>& connection)
{ 
  const auto& extra = std::static_pointer_cast<ql_template::Parser::TemplateExtra>(queryTemplate.getExtraData()); 
  const auto& var = queryTemplate.getTemplateVariables()[0];
  const auto& it = params.at(var.name);
  const auto& array_extra = std::static_pointer_cast<ql_template::Parser::ArrayVariableExtra>(var.extra);

  std::unordered_map<String,Void> new_params;
  orm::Executor::ParamsTypeMap paramsTypeMap;
  std::string values;

  if(it.valueType->classId.id == oatpp::Vector<Void>::Class::CLASS_ID.id)
    values = expandQuery(it.staticCast<Vector<Void>>(), array_extra,new_params,paramsTypeMap);
  else if(it.valueType->classId.id == oatpp::List<Void>::Class::CLASS_ID.id)
    values = expandQuery(it.staticCast<List<Void>>(), array_extra,new_params,paramsTypeMap);
  else if(it.valueType->classId.id == oatpp::UnorderedSet<Void>::Class::CLASS_ID.id)
    values = expandQuery(it.staticCast<UnorderedSet<Void>>(), array_extra,new_params,paramsTypeMap);
  
  const std::string& prepared_query = extra->preparedTemplate->std_str();
  const String& query = (prepared_query.substr(0, var.posStart) + values + prepared_query.substr(var.posStart + 2)).c_str();
  const auto& newQueryTemplate = parseQueryTemplate(extra->templateName + utils::conversion::uint64ToStr(new_params.size()), query, paramsTypeMap, extra->prepare);

  return executeQuery(newQueryTemplate, new_params, typeResolver, connection);
}

data::share::StringTemplate Executor::parseQueryTemplate(const oatpp::String& name,
                                                         const oatpp::String& text,
                                                         const ParamsTypeMap& paramsTypeMap,
                                                         bool prepare)
{

  auto&& t = ql_template::Parser::parseTemplate(text);

  auto extra = std::make_shared<ql_template::Parser::TemplateExtra>();
  t.setExtraData(extra);

  extra->prepare = prepare;
  extra->templateName = name;
  ql_template::TemplateValueProvider valueProvider;
  extra->preparedTemplate = t.format(&valueProvider);
  extra->paramsTypeMap = paramsTypeMap;

  return t;

}

std::shared_ptr<orm::Connection> Executor::getConnection() {
  auto connection = m_connectionProvider->get();
  if(connection) {
    return connection;
  }
  throw std::runtime_error("[oatpp::postgresql::Executor::getConnection()]: Error. Can't connect.");
}

std::shared_ptr<orm::QueryResult> Executor::execute(const StringTemplate& queryTemplate,
                                                    const std::unordered_map<oatpp::String, oatpp::Void>& params,
                                                    const std::shared_ptr<const data::mapping::TypeResolver>& typeResolver,
                                                    const std::shared_ptr<orm::Connection>& connection)
{

  std::shared_ptr<orm::Connection> conn = connection;
  if(!conn) {
    conn = getConnection();
  }

  std::shared_ptr<const data::mapping::TypeResolver> tr = typeResolver;
  if(!tr) {
    tr = m_defaultTypeResolver;
  }

  auto pgConnection = std::static_pointer_cast<postgresql::Connection>(conn);

  auto extra = std::static_pointer_cast<ql_template::Parser::TemplateExtra>(queryTemplate.getExtraData());
  bool prepare = extra->prepare;

  if(prepare) {

    if (!pgConnection->isPrepared(extra->templateName)) {
      auto result = prepareQuery(queryTemplate, tr, pgConnection);
      if(result->isSuccess()) {
        pgConnection->setPrepared(extra->templateName);
      } else {
        return result;
      }
    }

    return executeQueryPrepared(queryTemplate, params, tr, pgConnection);

  }
  if (queryTemplate.getTemplateVariables().size() == 1)
  {
    const auto& var = queryTemplate.getTemplateVariables()[0];
    if (std::static_pointer_cast<ql_template::Parser::ArrayVariableExtra>(var.extra) != nullptr)
      return expandAndExecuteQuery(queryTemplate,params,tr,pgConnection);
  }

  return executeQuery(queryTemplate, params, tr, pgConnection);

}

std::shared_ptr<orm::QueryResult> Executor::exec(const oatpp::String& statement,
                                                 const std::shared_ptr<orm::Connection>& connection,
                                                 bool useExecParams)
{

  std::shared_ptr<orm::Connection> conn = connection;
  if(!conn) {
    conn = getConnection();
  }

  auto pgConnection = std::static_pointer_cast<postgresql::Connection>(conn);

  PGresult *qres;
  if(useExecParams) {
    qres = PQexecParams(pgConnection->getHandle(),
                        statement->c_str(),
                        0 /* nParams */,
                        nullptr /* paramTypes */,
                        nullptr /* paramValues */,
                        nullptr /* paramLengths */,
                        nullptr /* paramFormats */,
                        1 /* resultFormat */);
  } else {
    qres = PQexec(pgConnection->getHandle(), statement->c_str());
  }

  return std::make_shared<QueryResult>(qres, pgConnection, m_connectionProvider, m_resultMapper, m_defaultTypeResolver);

}

std::shared_ptr<orm::QueryResult> Executor::begin(const std::shared_ptr<orm::Connection>& connection) {
  return exec("BEGIN", connection);
}

std::shared_ptr<orm::QueryResult> Executor::commit(const std::shared_ptr<orm::Connection>& connection) {
  if(!connection) {
    throw std::runtime_error("[oatpp::postgresql::Executor::commit()]: "
                             "Error. Can't COMMIT - NULL connection.");
  }
  return exec("COMMIT", connection);
}

std::shared_ptr<orm::QueryResult> Executor::rollback(const std::shared_ptr<orm::Connection>& connection) {
  if(!connection) {
    throw std::runtime_error("[oatpp::postgresql::Executor::commit()]: "
                             "Error. Can't ROLLBACK - NULL connection.");
  }
  return exec("ROLLBACK", connection);
}

oatpp::String Executor::getSchemaVersionTableName(const oatpp::String& suffix) {
  data::stream::BufferOutputStream stream;
  stream << "oatpp_schema_version";
  if (suffix && suffix->getSize() > 0) {
    stream << "_" << suffix;
  }
  return stream.toString();
}

std::shared_ptr<orm::QueryResult> Executor::updateSchemaVersion(v_int64 newVersion,
                                                                const oatpp::String& suffix,
                                                                const std::shared_ptr<orm::Connection>& connection)
{
  data::stream::BufferOutputStream stream;
  stream
    << "UPDATE "
    << getSchemaVersionTableName(suffix) << " "
    << "SET version=" << newVersion << ";";
  return exec(stream.toString(), connection, true);
}

v_int64 Executor::getSchemaVersion(const oatpp::String& suffix,
                                   const std::shared_ptr<orm::Connection>& connection)
{

  std::shared_ptr<orm::QueryResult> result;

  {
    data::stream::BufferOutputStream stream;
    stream << "CREATE TABLE IF NOT EXISTS " << getSchemaVersionTableName(suffix) << " (version BIGINT)";
    result = exec(stream.toString(), connection);
    if(!result->isSuccess()) {
      throw std::runtime_error("[oatpp::postgresql::Executor::getSchemaVersion()]: "
                               "Error. Can't create schema version table. " + result->getErrorMessage()->std_str());
    }
  }

  data::stream::BufferOutputStream stream;
  stream << "SELECT * FROM " << getSchemaVersionTableName(suffix);
  result = exec(stream.toString(), result->getConnection(), true);
  if(!result->isSuccess()) {
    throw std::runtime_error("[oatpp::postgresql::Executor::getSchemaVersion()]: "
                             "Error. Can't get schema version. " + result->getErrorMessage()->std_str());
  }

  auto rows = result->fetch<oatpp::Vector<oatpp::Object<VersionRow>>>();

  if(rows->size() == 0) {

    stream.setCurrentPosition(0);
    stream << "INSERT INTO " << getSchemaVersionTableName(suffix) << " (version) VALUES (0)";
    result = exec(stream.toString(), result->getConnection(), true);

    if(result->isSuccess()) {
      return 0;
    }

    throw std::runtime_error("[oatpp::postgresql::Executor::getSchemaVersion()]: "
                             "Error. Can't init schema version. " + result->getErrorMessage()->std_str());

  } else if(rows->size() == 1) {

    auto row = rows[0];
    if(!row->version) {
      throw std::runtime_error("[oatpp::postgresql::Executor::getSchemaVersion()]: "
                               "Error. The schema version table is corrupted - version is null.");
    }

    return row->version;

  }

  throw std::runtime_error("[oatpp::postgresql::Executor::getSchemaVersion()]: "
                           "Error. The schema version table is corrupted - multiple version rows.");

}

void Executor::migrateSchema(const oatpp::String& script,
                             v_int64 newVersion,
                             const oatpp::String& suffix,
                             const std::shared_ptr<orm::Connection>& connection)
{

  if(!script) {
    throw std::runtime_error("[oatpp::postgresql::Executor::migrateSchema()]: Error. Script is null.");
  }

  if(!connection) {
    throw std::runtime_error("[oatpp::postgresql::Executor::migrateSchema()]: Error. Connection is null.");
  }

  auto currVersion = getSchemaVersion(suffix, connection);
  if(newVersion <= currVersion) {
    return;
  }

  if(newVersion > currVersion + 1) {
    throw std::runtime_error("[oatpp::postgresql::Executor::migrateSchema()]: Error. +1 version increment is allowed only.");
  }

  if(script->getSize() == 0) {
    OATPP_LOGW("[oatpp::postgresql::Executor::migrateSchema()]", "Warning. Executing empty script for version %d", newVersion);
  }

  {

    orm::Transaction transaction(this, connection);

    std::shared_ptr<orm::QueryResult> result;

    result = exec(script, connection);
    if(!result->isSuccess()) {
      OATPP_LOGE("[oatpp::postgresql::Executor::migrateSchema()]",
                 "Error. Migration failed for version %d. %s", newVersion, result->getErrorMessage()->c_str());
      throw std::runtime_error("[oatpp::postgresql::Executor::migrateSchema()]: "
                               "Error. Migration failed. " + result->getErrorMessage()->std_str());

    }

    result = updateSchemaVersion(newVersion, suffix, connection);

    if(!result->isSuccess() || result->hasMoreToFetch() > 0) {
      throw std::runtime_error("[oatpp::postgresql::Executor::migrateSchema()]: Error. Migration failed. Can't set new version.");
    }

    result = transaction.commit();
    if(!result->isSuccess()) {
      throw std::runtime_error("[oatpp::postgresql::Executor::migrateSchema()]: Error. Migration failed. Can't commit.");
    }

  }

}

}}
