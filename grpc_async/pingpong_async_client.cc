/*
 * pingpong_async_client.cc
 *
 *  Created on: 20-Feb-2021
 *      Author: prateek
 */
#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include "pingpong.grpc.pb.h"

class PingPongClient
{
public:
	/* Constructor of PingPongClient
	 	 1. Initialize the stub by passing the channel
	 */
	explicit PingPongClient(std::shared_ptr<grpc::Channel> channel)
		: stub_ptr(pingpong::PingPong::NewStub(channel)){}

	/* Assembles the clients payload , sends it and presents the response back from the server */
	std::string SendPing(const std::string &msg)
	{
		// Data we want to send to server
		pingpong::PingRequest request;
		request.set_msg(msg);

		//Container for data we expect from the server
		pingpong::PingReply response;

		/*1. Context for the client.
		 *2. It could be used to convey extra information to the server and/or tweak certain behavior
		 */
		grpc::ClientContext context;

		/* The producer consumer queue we use to communicate asynchronously with the gRPC runtime */
		grpc::CompletionQueue cqueue;

		/* Storage for the status of the RPC upon completion */
		grpc::Status status;

		/*
		 * 1. stub->PrepareAyncSendPing() creates an RPC object, returning an instance to
		 *    store in "call" but does not actually start the RPC because we are using the
		 *    async API.
		 * 2. We need to hold on to the "call" instance in order to get updates on the ongoing RPC.
		 */
		std::unique_ptr<grpc::ClientAsyncResponseReader<pingpong::PingReply>> rpc(
				stub_ptr->PrepareAsyncSendPing(&context, request, &cqueue));

		/* StartCall initiates the RPC call */
		rpc->StartCall();

		/* 1. Request that, upon completion of the RPC, "reply" be updated with the server's response
		 * 2. "status" with the indication of whether the operation was successful.
		 * 3. Tag the request with integer 1 (tag to uniquely identify the request ).
		 */
		rpc->Finish(&response, &status, (void*)1);


		void *got_tag;
		bool ok = false;

		/*
		 * 1. Block until the next result is available in the completion queue.
		 * 2. The return value of Next should always be checked. This return value tells whether there is any kind of event
		 *    or the completion queue is hutting down.
		 */
		GPR_ASSERT(cqueue.Next(&got_tag, &ok));

		// Verify that the result from completion queue corresponds by its tag, our previous request
		GPR_ASSERT(got_tag == (void*) 1);

		// and the request was completed successfully. Note that ok correspond solely to the request for updates introduced by Finish()
		GPR_ASSERT(ok);

		//Act upon the status of the actual RPC
		if( status.ok() )
		{
			return response.msg();
		}
		else
		{
			return "RPC failed";
		}
	}


private:
	/* 1. Out of the passed grpc Channel comes the stub.
	 * 2. Our view of the server's exposed services
	 */
	std::unique_ptr<pingpong::PingPong::Stub> stub_ptr;
};


int main(int argc, char **argv)
{
	/* Instantiate the client.
	 * 1) It requires a channel, out of which the actual RPCs are created.
	 * 2) This channel models a connection to an end point.
	 * 3) We indicate that that the channel isn't authenticated.
	 */
	PingPongClient client(
			grpc::CreateChannel("0.0.0.0:50051", grpc::InsecureChannelCredentials())
	);

	//Invoke client member function calling the RPC
	std::string response = client.SendPing("Prateek");

	std::cout<<"Client received : "<< response << std::endl;

	return 0;
}



