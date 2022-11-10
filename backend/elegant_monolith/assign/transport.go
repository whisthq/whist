package assign

import (
	context "context"
	"fmt"
	"log"
	"net"

	"google.golang.org/grpc"
)

const PORT = "7730"

type AssignServer interface {
	Serve()
}

func NewServer(svc AssignService) (srv AssignServer) {
	return assignServer{
		service: svc,
	}
}

type assignServer struct {
	service AssignService
	UnimplementedAssignServiceServer
}

func (assignSrv *assignServer) MandelboxAssign(ctx context.Context, req *MandelboxAssignRequest) (res *MandelboxAssignResponse, err error) {
	log.Printf("Req: %v", req)
	mandelboxID, ip, err := assignSrv.service.MandelboxAssign(ctx, req.Regions, req.UserEmail, req.UserId, req.CommitHash)
	return &MandelboxAssignResponse{
		MandelboxId: mandelboxID,
		Ip:          ip,
		Error:       err.(AssignError).Code(),
	}, nil
}

func (assignSrv assignServer) Serve() {
	lis, err := net.Listen("tcp", fmt.Sprintf(":%s", PORT))
	if err != nil {
		log.Fatalf("failed to listen: %v", err)
	}
	s := grpc.NewServer()
	RegisterAssignServiceServer(s, &assignSrv)

	log.Printf("server listening at %v", lis.Addr())
	if err := s.Serve(lis); err != nil {
		log.Fatalf("failed to serve: %v", err)
	}
}
