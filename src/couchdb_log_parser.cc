// Copyright (c) 2019 Oleksandr Karaberov. All rights reserved.
// Use of this source code is governed by a MIT-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "couchdb_log_parser.h"

namespace logstat {

enum LogToken {
  LEVEL_TOK = 0,
  DATE_TOK,
  HOST_TOK,
  PID_TOK,
  MSG_ID_TOK,
  DOMAIN_TOK,
  IP_TOK,
  USER_TOK,
  METHOD_TOK,
  URL_TOK,
  HTTP_CODE_TOK,
  OK_TOK,
  TIME_TOK
};

template <typename TokType>
inline TokType ExtractToken(const LogToken index,
                            const std::vector<std::string> &tokens,
                            TokType def_value) {
  return tokens.size() > index ? tokens[index] : def_value;
};

// Special treatment for "strongly-typed" tokens such as HTTP methods, log
// severity levels, et cetera
template <>
inline log_utils::LogSeverityLevel ExtractToken<log_utils::LogSeverityLevel>(
    const LogToken index, const std::vector<std::string> &tokens,
    log_utils::LogSeverityLevel def_value) {
  return tokens.size() > index ? log_utils::GetLogLevel(tokens[index])
                               : def_value;
};

template <>
inline log_utils::HTTPMethod ExtractToken<log_utils::HTTPMethod>(
    const LogToken index, const std::vector<std::string> &tokens,
    log_utils::HTTPMethod def_value) {
  return tokens.size() > index ? log_utils::GetHTTPMethod(tokens[index])
                               : def_value;
};

template <>
inline std::time_t ExtractToken<std::time_t>(
    const LogToken index, const std::vector<std::string> &tokens,
    std::time_t def_value) {
  return tokens.size() > index
             ? log_utils::GetEpoch(tokens[index], kCouchdbLogDateFormat)
             : def_value;
};

Log CouchDBLogParser::ParseLog(const std::vector<std::string> &tokens) const {
  const auto level = ExtractToken<log_utils::LogSeverityLevel>(
      LEVEL_TOK, tokens, log_utils::LogSeverityLevel::LOG_UNDEF);
  const auto log_date = ExtractToken<std::time_t>(DATE_TOK, tokens, -1);
  Log couchdb_log = {};
  if (level != log_utils::LogSeverityLevel::LOG_UNDEF && log_date != -1) {
    couchdb_log = {.log_level = level,
                   .date = log_date,
                   .host = ExtractToken<std::string>(HOST_TOK, tokens, ""),
                   .pid = ExtractToken<std::string>(PID_TOK, tokens, "")};
    const auto http_method = ExtractToken<log_utils::HTTPMethod>(
        METHOD_TOK, tokens, log_utils::HTTPMethod::UNDEF_ERROR);
    const std::string msg_id =
        ExtractToken<std::string>(MSG_ID_TOK, tokens, kCouchdbNonReqMarker);

    if (msg_id.compare(kCouchdbNonReqMarker) == 0) {
      couchdb_log.is_generic_msg = true;
    } else if (http_method != log_utils::HTTPMethod::UNDEF_ERROR) {
      couchdb_log.domain_name =
          ExtractToken<std::string>(DOMAIN_TOK, tokens, "");
      couchdb_log.ip_address = ExtractToken<std::string>(IP_TOK, tokens, "");
      couchdb_log.user = ExtractToken<std::string>(USER_TOK, tokens, "");
      couchdb_log.http_code =
          ExtractToken<std::string>(HTTP_CODE_TOK, tokens, "");
      couchdb_log.req_time =
          std::stoi(ExtractToken<std::string>(TIME_TOK, tokens, "0"), nullptr);
      const auto req_url = ExtractToken<std::string>(URL_TOK, tokens, "");
      couchdb_log.request_url = req_url;
      couchdb_log.short_req_url = req_url.substr(0, req_url.find('?'));
      couchdb_log.http_method_str =
          ExtractToken<std::string>(METHOD_TOK, tokens, "");
    } else {
      couchdb_log.is_stacktrace = true;
    }
  }
  return couchdb_log;
};

}  // namespace logstat
