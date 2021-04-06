// Copyright Amazon.com Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"). You may
// not use this file except in compliance with the License. A copy of the
// License is located at
//
//	http://aws.amazon.com/apache2.0/
//
// or in the "license" file accompanying this file. This file is distributed
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
// express or implied. See the License for the specific language governing
// permissions and limitations under the License.

package main

import (
	"math/rand"
	"os"
	"time"

	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/app"
	"github.com/fractal/fractal/ecs-host-service/ecsagent/agent/logger"
)

func init() {
	rand.Seed(time.Now().UnixNano())
}

func main() {
	// This body really doesn't matter, since it's replaced by the ecs-host-service version anyways. It just needs to be nontrivial (e.g. call app.Run) and compile.
	logger.InitSeelog(nil)
	os.Exit(app.Run(nil, nil, nil))
}
