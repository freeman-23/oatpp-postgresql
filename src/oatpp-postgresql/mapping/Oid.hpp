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

#ifndef oatpp_postgresql_mapping_Oid_hpp
#define oatpp_postgresql_mapping_Oid_hpp

/***************************************************************************
 * The below OID table is copypasted from pg_type_d.h
 ***************************************************************************/

/*-------------------------------------------------------------------------
 *
 * pg_type_d.h
 *    Macro definitions for pg_type
 *
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * NOTES
 *  ******************************
 *  *** DO NOT EDIT THIS FILE! ***
 *  ******************************
 *
 *  It has been GENERATED by src/backend/catalog/genbki.pl
 *
 *-------------------------------------------------------------------------
 */

#define BOOLOID 16
#define BYTEAOID 17
#define CHAROID 18
#define NAMEOID 19
#define INT8OID 20
#define INT2OID 21
#define INT2VECTOROID 22
#define INT4OID 23
#define REGPROCOID 24
#define TEXTOID 25
#define OIDOID 26
#define TIDOID 27
#define XIDOID 28
#define CIDOID 29
#define OIDVECTOROID 30
#define JSONOID 114
#define XMLOID 142
#define PGNODETREEOID 194
#define PGNDISTINCTOID 3361
#define PGDEPENDENCIESOID 3402
#define PGMCVLISTOID 5017
#define PGDDLCOMMANDOID 32
#define POINTOID 600
#define LSEGOID 601
#define PATHOID 602
#define BOXOID 603
#define POLYGONOID 604
#define LINEOID 628
#define FLOAT4OID 700
#define FLOAT8OID 701
#define UNKNOWNOID 705
#define CIRCLEOID 718
#define CASHOID 790
#define MACADDROID 829
#define INETOID 869
#define CIDROID 650
#define MACADDR8OID 774
#define ACLITEMOID 1033
#define BPCHAROID 1042
#define VARCHAROID 1043
#define DATEOID 1082
#define TIMEOID 1083
#define TIMESTAMPOID 1114
#define TIMESTAMPTZOID 1184
#define INTERVALOID 1186
#define TIMETZOID 1266
#define BITOID 1560
#define VARBITOID 1562
#define NUMERICOID 1700
#define REFCURSOROID 1790
#define REGPROCEDUREOID 2202
#define REGOPEROID 2203
#define REGOPERATOROID 2204
#define REGCLASSOID 2205
#define REGTYPEOID 2206
#define REGROLEOID 4096
#define REGNAMESPACEOID 4089
#define UUIDOID 2950
#define LSNOID 3220
#define TSVECTOROID 3614
#define GTSVECTOROID 3642
#define TSQUERYOID 3615
#define REGCONFIGOID 3734
#define REGDICTIONARYOID 3769
#define JSONBOID 3802
#define JSONPATHOID 4072
#define TXID_SNAPSHOTOID 2970
#define INT4RANGEOID 3904
#define NUMRANGEOID 3906
#define TSRANGEOID 3908
#define TSTZRANGEOID 3910
#define DATERANGEOID 3912
#define INT8RANGEOID 3926
#define RECORDOID 2249
#define RECORDARRAYOID 2287
#define CSTRINGOID 2275
#define ANYOID 2276
#define ANYARRAYOID 2277
#define VOIDOID 2278
#define TRIGGEROID 2279
#define EVTTRIGGEROID 3838
#define LANGUAGE_HANDLEROID 2280
#define INTERNALOID 2281
#define OPAQUEOID 2282
#define ANYELEMENTOID 2283
#define ANYNONARRAYOID 2776
#define ANYENUMOID 3500
#define FDW_HANDLEROID 3115
#define INDEX_AM_HANDLEROID 325
#define TSM_HANDLEROID 3310
#define TABLE_AM_HANDLEROID 269
#define ANYRANGEOID 3831
#define BOOLARRAYOID 1000
#define BYTEAARRAYOID 1001
#define CHARARRAYOID 1002
#define NAMEARRAYOID 1003
#define INT8ARRAYOID 1016
#define INT2ARRAYOID 1005
#define INT2VECTORARRAYOID 1006
#define INT4ARRAYOID 1007
#define REGPROCARRAYOID 1008
#define TEXTARRAYOID 1009
#define OIDARRAYOID 1028
#define TIDARRAYOID 1010
#define XIDARRAYOID 1011
#define CIDARRAYOID 1012
#define OIDVECTORARRAYOID 1013
#define JSONARRAYOID 199
#define XMLARRAYOID 143
#define POINTARRAYOID 1017
#define LSEGARRAYOID 1018
#define PATHARRAYOID 1019
#define BOXARRAYOID 1020
#define POLYGONARRAYOID 1027
#define LINEARRAYOID 629
#define FLOAT4ARRAYOID 1021
#define FLOAT8ARRAYOID 1022
#define CIRCLEARRAYOID 719
#define MONEYARRAYOID 791
#define MACADDRARRAYOID 1040
#define INETARRAYOID 1041
#define CIDRARRAYOID 651
#define MACADDR8ARRAYOID 775
#define ACLITEMARRAYOID 1034
#define BPCHARARRAYOID 1014
#define VARCHARARRAYOID 1015
#define DATEARRAYOID 1182
#define TIMEARRAYOID 1183
#define TIMESTAMPARRAYOID 1115
#define TIMESTAMPTZARRAYOID 1185
#define INTERVALARRAYOID 1187
#define TIMETZARRAYOID 1270
#define BITARRAYOID 1561
#define VARBITARRAYOID 1563
#define NUMERICARRAYOID 1231
#define REFCURSORARRAYOID 2201
#define REGPROCEDUREARRAYOID 2207
#define REGOPERARRAYOID 2208
#define REGOPERATORARRAYOID 2209
#define REGCLASSARRAYOID 2210
#define REGTYPEARRAYOID 2211
#define REGROLEARRAYOID 4097
#define REGNAMESPACEARRAYOID 4090
#define UUIDARRAYOID 2951
#define PG_LSNARRAYOID 3221
#define TSVECTORARRAYOID 3643
#define GTSVECTORARRAYOID 3644
#define TSQUERYARRAYOID 3645
#define REGCONFIGARRAYOID 3735
#define REGDICTIONARYARRAYOID 3770
#define JSONBARRAYOID 3807
#define JSONPATHARRAYOID 4073
#define TXID_SNAPSHOTARRAYOID 2949
#define INT4RANGEARRAYOID 3905
#define NUMRANGEARRAYOID 3907
#define TSRANGEARRAYOID 3909
#define TSTZRANGEARRAYOID 3911
#define DATERANGEARRAYOID 3913
#define INT8RANGEARRAYOID 3927
#define CSTRINGARRAYOID 1263

#endif // oatpp_postgresql_mapping_Oid_hpp
