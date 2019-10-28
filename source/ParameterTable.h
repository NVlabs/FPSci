/***************************************************************************
# Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#pragma once

#include <G3D/G3D.h>
#include <time.h>
#include <vector>
#include <map>
#include <stdint.h>
#include "TargetEntity.h"

/** Struct for condition parameter or experiment description */
struct ParameterTable
{
	std::map<std::string, float> val;
	std::map<std::string, std::string> str;
	std::map<std::string, std::vector<float>> val_vec;
	Array<Destination> destinations;
	AABox bounds;

	void add(std::string s, float f) {
		val.insert(std::pair<std::string, float>(s, f));
	}
	void add(std::string s, std::vector<float> v) {
		val_vec.insert(std::pair<std::string, std::vector<float>>(s, v));
	}
	void add(std::string str_key, std::string str_val) {
		str.insert(std::pair<std::string, std::string>(str_key, str_val));
	}

	void add(Array<Destination> dests) {
		destinations = dests;
	}

	ParameterTable() {
		val = std::map<std::string, float>();
		str = std::map<std::string, std::string>();
		val_vec = std::map<std::string, std::vector<float>>();
	}
};
