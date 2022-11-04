// Code generated by protoc-gen-go. DO NOT EDIT.
// versions:
// 	protoc-gen-go v1.28.1
// 	protoc        v3.20.1
// source: assign/assign.proto

package assign

import (
	protoreflect "google.golang.org/protobuf/reflect/protoreflect"
	protoimpl "google.golang.org/protobuf/runtime/protoimpl"
	reflect "reflect"
	sync "sync"
)

const (
	// Verify that this generated code is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(20 - protoimpl.MinVersion)
	// Verify that runtime/protoimpl is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(protoimpl.MaxVersion - 20)
)

type MandelboxAssignRequest struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	Regions []string `protobuf:"bytes,1,rep,name=regions,proto3" json:"regions,omitempty"`
}

func (x *MandelboxAssignRequest) Reset() {
	*x = MandelboxAssignRequest{}
	if protoimpl.UnsafeEnabled {
		mi := &file_assign_assign_proto_msgTypes[0]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *MandelboxAssignRequest) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*MandelboxAssignRequest) ProtoMessage() {}

func (x *MandelboxAssignRequest) ProtoReflect() protoreflect.Message {
	mi := &file_assign_assign_proto_msgTypes[0]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use MandelboxAssignRequest.ProtoReflect.Descriptor instead.
func (*MandelboxAssignRequest) Descriptor() ([]byte, []int) {
	return file_assign_assign_proto_rawDescGZIP(), []int{0}
}

func (x *MandelboxAssignRequest) GetRegions() []string {
	if x != nil {
		return x.Regions
	}
	return nil
}

type MandelboxAssignResponse struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	MandelboxId string `protobuf:"bytes,1,opt,name=mandelbox_id,json=mandelboxId,proto3" json:"mandelbox_id,omitempty"`
	Ip          string `protobuf:"bytes,2,opt,name=ip,proto3" json:"ip,omitempty"`
	Err         string `protobuf:"bytes,3,opt,name=err,proto3" json:"err,omitempty"`
}

func (x *MandelboxAssignResponse) Reset() {
	*x = MandelboxAssignResponse{}
	if protoimpl.UnsafeEnabled {
		mi := &file_assign_assign_proto_msgTypes[1]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *MandelboxAssignResponse) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*MandelboxAssignResponse) ProtoMessage() {}

func (x *MandelboxAssignResponse) ProtoReflect() protoreflect.Message {
	mi := &file_assign_assign_proto_msgTypes[1]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use MandelboxAssignResponse.ProtoReflect.Descriptor instead.
func (*MandelboxAssignResponse) Descriptor() ([]byte, []int) {
	return file_assign_assign_proto_rawDescGZIP(), []int{1}
}

func (x *MandelboxAssignResponse) GetMandelboxId() string {
	if x != nil {
		return x.MandelboxId
	}
	return ""
}

func (x *MandelboxAssignResponse) GetIp() string {
	if x != nil {
		return x.Ip
	}
	return ""
}

func (x *MandelboxAssignResponse) GetErr() string {
	if x != nil {
		return x.Err
	}
	return ""
}

var File_assign_assign_proto protoreflect.FileDescriptor

var file_assign_assign_proto_rawDesc = []byte{
	0x0a, 0x13, 0x61, 0x73, 0x73, 0x69, 0x67, 0x6e, 0x2f, 0x61, 0x73, 0x73, 0x69, 0x67, 0x6e, 0x2e,
	0x70, 0x72, 0x6f, 0x74, 0x6f, 0x12, 0x10, 0x63, 0x6f, 0x6d, 0x2e, 0x77, 0x68, 0x69, 0x73, 0x74,
	0x2e, 0x61, 0x73, 0x73, 0x69, 0x67, 0x6e, 0x22, 0x32, 0x0a, 0x16, 0x4d, 0x61, 0x6e, 0x64, 0x65,
	0x6c, 0x62, 0x6f, 0x78, 0x41, 0x73, 0x73, 0x69, 0x67, 0x6e, 0x52, 0x65, 0x71, 0x75, 0x65, 0x73,
	0x74, 0x12, 0x18, 0x0a, 0x07, 0x72, 0x65, 0x67, 0x69, 0x6f, 0x6e, 0x73, 0x18, 0x01, 0x20, 0x03,
	0x28, 0x09, 0x52, 0x07, 0x72, 0x65, 0x67, 0x69, 0x6f, 0x6e, 0x73, 0x22, 0x5e, 0x0a, 0x17, 0x4d,
	0x61, 0x6e, 0x64, 0x65, 0x6c, 0x62, 0x6f, 0x78, 0x41, 0x73, 0x73, 0x69, 0x67, 0x6e, 0x52, 0x65,
	0x73, 0x70, 0x6f, 0x6e, 0x73, 0x65, 0x12, 0x21, 0x0a, 0x0c, 0x6d, 0x61, 0x6e, 0x64, 0x65, 0x6c,
	0x62, 0x6f, 0x78, 0x5f, 0x69, 0x64, 0x18, 0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x0b, 0x6d, 0x61,
	0x6e, 0x64, 0x65, 0x6c, 0x62, 0x6f, 0x78, 0x49, 0x64, 0x12, 0x0e, 0x0a, 0x02, 0x69, 0x70, 0x18,
	0x02, 0x20, 0x01, 0x28, 0x09, 0x52, 0x02, 0x69, 0x70, 0x12, 0x10, 0x0a, 0x03, 0x65, 0x72, 0x72,
	0x18, 0x03, 0x20, 0x01, 0x28, 0x09, 0x52, 0x03, 0x65, 0x72, 0x72, 0x32, 0x79, 0x0a, 0x0d, 0x41,
	0x73, 0x73, 0x69, 0x67, 0x6e, 0x53, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x12, 0x68, 0x0a, 0x0f,
	0x4d, 0x61, 0x6e, 0x64, 0x65, 0x6c, 0x62, 0x6f, 0x78, 0x41, 0x73, 0x73, 0x69, 0x67, 0x6e, 0x12,
	0x28, 0x2e, 0x63, 0x6f, 0x6d, 0x2e, 0x77, 0x68, 0x69, 0x73, 0x74, 0x2e, 0x61, 0x73, 0x73, 0x69,
	0x67, 0x6e, 0x2e, 0x4d, 0x61, 0x6e, 0x64, 0x65, 0x6c, 0x62, 0x6f, 0x78, 0x41, 0x73, 0x73, 0x69,
	0x67, 0x6e, 0x52, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x1a, 0x29, 0x2e, 0x63, 0x6f, 0x6d, 0x2e,
	0x77, 0x68, 0x69, 0x73, 0x74, 0x2e, 0x61, 0x73, 0x73, 0x69, 0x67, 0x6e, 0x2e, 0x4d, 0x61, 0x6e,
	0x64, 0x65, 0x6c, 0x62, 0x6f, 0x78, 0x41, 0x73, 0x73, 0x69, 0x67, 0x6e, 0x52, 0x65, 0x73, 0x70,
	0x6f, 0x6e, 0x73, 0x65, 0x22, 0x00, 0x42, 0x2c, 0x5a, 0x2a, 0x67, 0x69, 0x74, 0x68, 0x75, 0x62,
	0x2e, 0x63, 0x6f, 0x6d, 0x2f, 0x77, 0x68, 0x69, 0x73, 0x74, 0x68, 0x71, 0x2f, 0x62, 0x61, 0x63,
	0x6b, 0x65, 0x6e, 0x64, 0x2f, 0x73, 0x65, 0x72, 0x76, 0x69, 0x63, 0x65, 0x73, 0x2f, 0x61, 0x73,
	0x73, 0x69, 0x67, 0x6e, 0x62, 0x06, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x33,
}

var (
	file_assign_assign_proto_rawDescOnce sync.Once
	file_assign_assign_proto_rawDescData = file_assign_assign_proto_rawDesc
)

func file_assign_assign_proto_rawDescGZIP() []byte {
	file_assign_assign_proto_rawDescOnce.Do(func() {
		file_assign_assign_proto_rawDescData = protoimpl.X.CompressGZIP(file_assign_assign_proto_rawDescData)
	})
	return file_assign_assign_proto_rawDescData
}

var file_assign_assign_proto_msgTypes = make([]protoimpl.MessageInfo, 2)
var file_assign_assign_proto_goTypes = []interface{}{
	(*MandelboxAssignRequest)(nil),  // 0: com.whist.assign.MandelboxAssignRequest
	(*MandelboxAssignResponse)(nil), // 1: com.whist.assign.MandelboxAssignResponse
}
var file_assign_assign_proto_depIdxs = []int32{
	0, // 0: com.whist.assign.AssignService.MandelboxAssign:input_type -> com.whist.assign.MandelboxAssignRequest
	1, // 1: com.whist.assign.AssignService.MandelboxAssign:output_type -> com.whist.assign.MandelboxAssignResponse
	1, // [1:2] is the sub-list for method output_type
	0, // [0:1] is the sub-list for method input_type
	0, // [0:0] is the sub-list for extension type_name
	0, // [0:0] is the sub-list for extension extendee
	0, // [0:0] is the sub-list for field type_name
}

func init() { file_assign_assign_proto_init() }
func file_assign_assign_proto_init() {
	if File_assign_assign_proto != nil {
		return
	}
	if !protoimpl.UnsafeEnabled {
		file_assign_assign_proto_msgTypes[0].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*MandelboxAssignRequest); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_assign_assign_proto_msgTypes[1].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*MandelboxAssignResponse); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
	}
	type x struct{}
	out := protoimpl.TypeBuilder{
		File: protoimpl.DescBuilder{
			GoPackagePath: reflect.TypeOf(x{}).PkgPath(),
			RawDescriptor: file_assign_assign_proto_rawDesc,
			NumEnums:      0,
			NumMessages:   2,
			NumExtensions: 0,
			NumServices:   1,
		},
		GoTypes:           file_assign_assign_proto_goTypes,
		DependencyIndexes: file_assign_assign_proto_depIdxs,
		MessageInfos:      file_assign_assign_proto_msgTypes,
	}.Build()
	File_assign_assign_proto = out.File
	file_assign_assign_proto_rawDesc = nil
	file_assign_assign_proto_goTypes = nil
	file_assign_assign_proto_depIdxs = nil
}