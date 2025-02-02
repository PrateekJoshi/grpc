HOST_SYSTEM = $(shell uname | cut -f 1 -d_)
SYSTEM ?= $(HOST_SYSTEM)
CXX = g++
CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++11 -I/root/.local/include/
ifeq ($(SYSTEM),Darwin)
LDFLAGS += -L/usr/local/lib `pkg-config --libs protobuf grpc++`\
           -pthread\
           -lgrpc++_reflection\
           -ldl
else
LDFLAGS += -L/usr/local/lib `pkg-config --libs protobuf grpc++`\
           -pthread\
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
           -ldl
endif
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`
PROTOS_PATH = ./proto

####################################################
# Build these targets 
####################################################
all:system-check proto_to_grpc_cpp pingpong_async_server  pingpong_async_client pingpong_async_client_mulithreaded

############################ 
#proto to cpp files
###########################
proto_to_grpc_cpp : 
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) pingpong.proto 
	$(PROTOC) -I $(PROTOS_PATH) --proto_path=$(PROTOS_PATH)  --cpp_out=. $(PROTOS_PATH)/pingpong.proto	

###########################
# grpc cpp to object files
###########################
pingpong.pb.o: pingpong.pb.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^ 

pingpong.grpc.pb.o: pingpong.grpc.pb.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^ 

#################################
# Server and Client object files
#################################
pingpong_async_server.o : pingpong_async_server.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^
	
pingpong_async_client.o : pingpong_async_client.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^

pingpong_async_client_mulithreaded.o : pingpong_async_client_mulithreaded.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $^

###############################################################################
# Generate Server and Client binaries by linking objects files and shared libs
###############################################################################
pingpong_async_server : pingpong.pb.o pingpong.grpc.pb.o pingpong_async_server.o 
	$(CXX) $^ $(LDFLAGS) -o $@ 

pingpong_async_client: pingpong.pb.o pingpong.grpc.pb.o pingpong_async_client.o 
	$(CXX) $^ $(LDFLAGS) -o $@ 

pingpong_async_client_mulithreaded : pingpong.pb.o pingpong.grpc.pb.o pingpong_async_client_mulithreaded.o 
	$(CXX) $^ $(LDFLAGS) -o $@ 

clean:
	rm -f *.o *.pb.cc *.pb.h pingpong_async_server pingpong_async_client pingpong_async_client_mulithreaded
	
# The following is to test your system and ensure a smoother experience.
# They are by no means necessary to actually compile a grpc-enabled software.

PROTOC_CMD = which $(PROTOC)
PROTOC_CHECK_CMD = $(PROTOC) --version | grep -q libprotoc.3
PLUGIN_CHECK_CMD = which $(GRPC_CPP_PLUGIN)
HAS_PROTOC = $(shell $(PROTOC_CMD) > /dev/null && echo true || echo false)
ifeq ($(HAS_PROTOC),true)
HAS_VALID_PROTOC = $(shell $(PROTOC_CHECK_CMD) 2> /dev/null && echo true || echo false)
endif
HAS_PLUGIN = $(shell $(PLUGIN_CHECK_CMD) > /dev/null && echo true || echo false)

SYSTEM_OK = false
ifeq ($(HAS_VALID_PROTOC),true)
ifeq ($(HAS_PLUGIN),true)
SYSTEM_OK = true
endif
endif

system-check:
ifneq ($(HAS_VALID_PROTOC),true)
	@echo " DEPENDENCY ERROR"
	@echo
	@echo "You don't have protoc 3.0.0 installed in your path."
	@echo "Please install Google protocol buffers 3.0.0 and its compiler."
	@echo "You can find it here:"
	@echo
	@echo "   https://github.com/google/protobuf/releases/tag/v3.0.0"
	@echo
	@echo "Here is what I get when trying to evaluate your version of protoc:"
	@echo
	-$(PROTOC) --version
	@echo
	@echo
endif
ifneq ($(HAS_PLUGIN),true)
	@echo " DEPENDENCY ERROR"
	@echo
	@echo "You don't have the grpc c++ protobuf plugin installed in your path."
	@echo "Please install grpc. You can find it here:"
	@echo
	@echo "   https://github.com/grpc/grpc"
	@echo
	@echo "Here is what I get when trying to detect if you have the plugin:"
	@echo
	-which $(GRPC_CPP_PLUGIN)
	@echo
	@echo
endif
ifneq ($(SYSTEM_OK),true)
	@false
endif