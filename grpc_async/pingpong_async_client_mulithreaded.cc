/*
 * pingpong_async_client_mulithreaded.cc
 *
 *  Created on: 21-Feb-2021
 *      Author: prateek
 * Refer : https://github.com/grpc/grpc/blob/v1.35.0/examples/cpp/helloworld/greeter_async_client2.cc
 *
 * Client :
 * 1. Main thread 	: In loop , sending RPC in every iteration
 * 2. Reader thread	: Listen on completion queue for response from server
 *
 */
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include "pingpong.grpc.pb.h"

class PingPongClient
{
public:
	// Constructor ( initialize stub with given channel )
	explicit PingPongClient(std::shared_ptr<grpc::Channel> channel)
		: stub_ptr(pingpong::PingPong::NewStub(channel)) {}

	/* Writer thread (main thread ): Funtion which will send the RPC
	 * Assemble client payload and send it to the server
	 */
	void sendMessage(const std::string &msg)
	{
		//Data we are sending to the server
		pingpong::PingRequest request;
		request.set_msg(msg);

		// Call object to store RPC data
		AsyncClientCall *call = new AsyncClientCall;

		/*
		 * 1. stub->PrepareAyncSendPing() creates an RPC object, returning an instance to
		 *    store in "call" but does not actually start the RPC because we are using the
		 *    async API.
		 * 2. We need to hold on to the "call" instance in order to get updates on the ongoing RPC.
		 */
		call->response_reader = stub_ptr->PrepareAsyncSendPing(&call->ctx, request, &cqueue);

		//Start call initiates RPC call
		call->response_reader->StartCall();

		/* 1. Request that, upon completion of the RPC, "reply" be updated with the server's response
		 * 2. "status" with the indication of whether the operation was successful.
		 * 3. Tag the request with memory address of the call object ( to uniquely identigy each request )
		 */
		call->response_reader->Finish(&call->response, &call->status, (void*) call);
	}

	/* READER THREAD
	 * 1) Loop while listening for completed responses
	 * 2) Prints out the response from the server
	 */
	void AsyncCompleteRPC()
	{
		void *got_tag;
		bool ok = false;

		// Block until the next result is available in the completion queue "cqueue"
		while( cqueue.Next(&got_tag, &ok))
		{
			// The tag in this example is memory location of call object
			AsyncClientCall *call = static_cast<AsyncClientCall*>(got_tag);

			// Verify that the request was completed successfully. Note that "ok"
			// corresponds solely to the request for updates introduced by Finish().
			GPR_ASSERT(ok);

			if( call->status.ok() )
			{
				std::cout<<"Greeter received: "<<call->response.msg()<< std::endl;
			}
			else
			{
				std::cout<<"RPC failed"<<std::endl;
			}

			//Once we're complete, deallocate the call object
			delete call;
		}
	}

private:
	//struct for keeping state and data information for each request
	struct AsyncClientCall
	{
		//Container for data we expect from server
		pingpong::PingReply response;

		//Context for the client.
		//It could be used to convey extra information to the server and/or tweak certain RPC behavior
		grpc::ClientContext ctx;

		//Storage for status of RPC upon completion
		grpc::Status status;

		//Response reader
		std::unique_ptr<grpc::ClientAsyncResponseReader<pingpong::PingReply>> response_reader;
	};


	/* 1. Out of the passed grpc Channel comes the stub.
	 * 2. Our view of the server's exposed services
	 */
	std::unique_ptr<pingpong::PingPong::Stub> stub_ptr;

	/* The producer-consumer queue we use to communicate asynchronously with the gRPC runtime */
	grpc::CompletionQueue cqueue;
};

int main(int argc, char **argv)
{
	/* Instantiate the client.
		 * 1) It requires a channel, out of which the actual RPCs are created.
		 * 2) This channel models a connection to an end point.
		 * 3) We indicate that that the channel isn't authenticated.
	*/
	PingPongClient client( grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

	// Spawn REDAER THREAD that loops indefinitely
	std::thread reader_thread = std::thread(&PingPongClient::AsyncCompleteRPC, &client);

	// Send multiple requests in loop
	for( int i = 0 ; i < 100000 ; i++ )
	{
		std::string msg("Prateek "+ std::to_string(i));
		client.sendMessage(msg);		// Call RPC
	}

	std::cout<<"Press control-c to quit <<std::endl"<<std::endl;

	//Block forever
	reader_thread.join();

	return 0;
}




