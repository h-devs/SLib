#pragma once
/*
* Copyright 2016 Nu-book Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifdef ANDROID

#include <sstream>
#include <cstdlib>

template <typename T>
inline std::string std_to_string(T x) {
	std::stringstream buf;
	buf << x;
	return buf.str();
}

template <typename T>
inline std::wstring std_to_wstring(T x) {
	std::wstringstream buf;
	buf << x;
	return buf.str();
}

inline int std_stoi(const std::string& s) {
	return atoi(s.c_str());
}

inline int std_stoll(const std::string& s) {
	return atoll(s.c_str());
}

#else

#define std_to_string std::to_string
#define std_to_wstring std::to_wstring
#define std_stoi std::stoi
#define std_stoll std::stoll

#endif
