#!/usr/bin/env bash
# (C) Copyright 2020-2021 Hewlett Packard Enterprise Development LP
#
# 
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at
#
# 
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# 
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.

#arguments: $1 = message_to_send; $2 = listener_hostname; $3 = listener_port; $4 = listener_trust_domain; $5 = listener_workload_id
cd /mnt/c-spiffe/integration_test/helpers/go-echo-server/client && su - client-workload -c "./go-client '$1' $2 $3 $4 $5"
