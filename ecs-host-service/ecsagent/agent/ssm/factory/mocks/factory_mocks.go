// Copyright Amazon.com Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License"). You may
// not use this file except in compliance with the License. A copy of the
// License is located at
//
//     http://aws.amazon.com/apache2.0/
//
// or in the "license" file accompanying this file. This file is distributed
// on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
// express or implied. See the License for the specific language governing
// permissions and limitations under the License.
//

// Code generated by MockGen. DO NOT EDIT.
// Source: github.com/fractal/fractal/ecs-host-service/ecsagent/agent/ssm/factory (interfaces: SSMClientCreator)

// Package mock_factory is a generated GoMock package.
package mock_factory

import (
	reflect "reflect"

	credentials "github.com/fractal/fractal/ecs-host-service/ecsagent/agent/credentials"
	ssm "github.com/fractal/fractal/ecs-host-service/ecsagent/agent/ssm"
	gomock "github.com/golang/mock/gomock"
)

// MockSSMClientCreator is a mock of SSMClientCreator interface
type MockSSMClientCreator struct {
	ctrl     *gomock.Controller
	recorder *MockSSMClientCreatorMockRecorder
}

// MockSSMClientCreatorMockRecorder is the mock recorder for MockSSMClientCreator
type MockSSMClientCreatorMockRecorder struct {
	mock *MockSSMClientCreator
}

// NewMockSSMClientCreator creates a new mock instance
func NewMockSSMClientCreator(ctrl *gomock.Controller) *MockSSMClientCreator {
	mock := &MockSSMClientCreator{ctrl: ctrl}
	mock.recorder = &MockSSMClientCreatorMockRecorder{mock}
	return mock
}

// EXPECT returns an object that allows the caller to indicate expected use
func (m *MockSSMClientCreator) EXPECT() *MockSSMClientCreatorMockRecorder {
	return m.recorder
}

// NewSSMClient mocks base method
func (m *MockSSMClientCreator) NewSSMClient(arg0 string, arg1 credentials.IAMRoleCredentials) ssm.SSMClient {
	m.ctrl.T.Helper()
	ret := m.ctrl.Call(m, "NewSSMClient", arg0, arg1)
	ret0, _ := ret[0].(ssm.SSMClient)
	return ret0
}

// NewSSMClient indicates an expected call of NewSSMClient
func (mr *MockSSMClientCreatorMockRecorder) NewSSMClient(arg0, arg1 interface{}) *gomock.Call {
	mr.mock.ctrl.T.Helper()
	return mr.mock.ctrl.RecordCallWithMethodType(mr.mock, "NewSSMClient", reflect.TypeOf((*MockSSMClientCreator)(nil).NewSSMClient), arg0, arg1)
}
