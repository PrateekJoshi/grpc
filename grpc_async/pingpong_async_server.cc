/*
 * pingpong_async_server.cc
 *
 *  Created on: 20-Feb-2021
 *      Author: prateek
 */
#include <memory>
#include <iostream>
#include <string>
#include <thread>
#include <grpcpp/grpcpp.h>
#include <grpc/support/log.h>
#include "pingpong.grpc.pb.h"

class ServerImpl final
{
private:
	// gRPC Server pointer
	std::unique_ptr<grpc::Server> server_ptr;

	//PingPong async service object
	pingpong::PingPong::AsyncService service;

	//gRPC Completion queue pointer
	std::unique_ptr<grpc::ServerCompletionQueue> completion_queue_ptr;

	/*
	 * Class encompassing the state and logic needed to serve a request
	 * A new instance of this call is created for a new client call
	 */
	class CallData
	{
	public:
		/* The constructor of this call takes 2 parameters :
		 * 	1) Service instance ( in our case asynchronous service)
		 * 	2) Completion queue used for asynchronous communication with gRPC runtime
		 */
		CallData(pingpong::PingPong::AsyncService *async_service, grpc::ServerCompletionQueue *cqueue)
			: calldata_service_ptr(async_service) , calldata_completion_queue_ptr(cqueue), responder(&ctx), status(CREATE)
		{
			// Invoke the serving logic right away as soon as this instance is created
			Proceed();
		}


		/* Async request serving logic
		 * NOTE -> Each request we call this member function in its own CallData instance
		 */
		void Proceed()
		{
			if( status == CREATE )
			{
				// Make this instance progress to the PROCESS state.
				status = PROCESS;

				/* 1. As a part of initial CREATE state , we "request" the system to start processing RequestSendPing requests.
				 * 2. In this request, "this" acts as a tag to uniquely identify the request ( so that different CallData instances can serve different
				 * requests concurrently), in this case the memory address of this CallData instance.
				 */
				calldata_service_ptr->RequestSendPing(&ctx, &request, &responder, calldata_completion_queue_ptr,calldata_completion_queue_ptr, this);
			}
			else if( status == PROCESS )
			{
				/* 1. Spawn a new CallData instance to serve new clients while we process the one for this CallData.
				 * 2. The insatnce will deallocate itself as part of its FINISH state
				 */
				new CallData(calldata_service_ptr, calldata_completion_queue_ptr);

				/** Actual Processing **/
				std::string prefix("Hello");
				response.set_msg(prefix + request.msg());

				/* 1) Now we are DONE.
				 * 2) Let the gRPC runtime know we have finished, using the memory address of this instance as the uniquely identifying tag for the event
				 */
				status = FINISH;
				responder.Finish(response, grpc::Status::OK, this);
			}
			else
			{
				GPR_ASSERT( status == FINISH );
				// Once in the FINISH state, deallocate ourselves ( CallData)
				delete this;
			}
		}


	private:
		// Means of communication with gRPC runtime for an async service
		pingpong::PingPong::AsyncService *calldata_service_ptr;

		// The producer-consumer queue for async server notifications
		grpc::ServerCompletionQueue *calldata_completion_queue_ptr;

		/* Context for RPC , allowing to tweak aspects of it such as the use of compression, authentication, as well as to send metadata back
		 * to the client
		 */
		grpc::ServerContext ctx;

		//Client request data type	( this we will get from client )
		pingpong::PingRequest request;

		//Client response data type ( this we will send back to the client )
		pingpong::PingReply response;

		// The means to get back to the client
		grpc::ServerAsyncResponseWriter<pingpong::PingReply> responder;

		// Create enum to store state of the RPC call
		enum CallStatus { CREATE , PROCESS, FINISH };

		CallStatus status;

	};

	/* Function to handler RPCs. It is the Server main LOOP
	 * This can be run in multiple threads if needed
	 */
	void handleRPCs()
	{
		// Spawn a new CallData instance to serve new clients
		new CallData(&service, completion_queue_ptr.get());

		// Tag to uniquely identify the request
		void *tag;

		bool ok;

		while(true)
		{
			/* 1. Block waiting to read the next event from the completion queue.
			 * 2. The event is uniquely identified by its tag, which is this case is the memory address of the CallData instance.
			 * 3. The return value of Next should always be checked. This return value tells us whether there is any kind of event of completion queue is
			 *    shutting down.
			 */
			GPR_ASSERT(completion_queue_ptr->Next(&tag, &ok));
			GPR_ASSERT(ok);
			static_cast<CallData*>(tag)->Proceed();
		}

	}


public:
	/* Destructor */
	~ServerImpl()
	{
		//shutdown server
		server_ptr->Shutdown();

		//shutdown completion queue
		completion_queue_ptr->Shutdown();
	}

	/*
	 * Create the gRPC server , bind the service implementation and run the Server
	 */
	void run()
	{
		// server address
		std::string server_addr("0.0.0.0:50051");

		/* Create a ServerBuilder object */
		grpc::ServerBuilder builder;

		//Listen on the given address without any authentication mechanism
		builder.AddListeningPort(server_addr, grpc::InsecureServerCredentials());

		// Register service_ptr as the instance through which we will communicate with
		// clients. In this case it corresponds to an "Asynchronous" service
		builder.RegisterService(&service);

		// Get pointer to completion queue used for the asynchronous communication with the
		// gRPC runtime
		completion_queue_ptr = builder.AddCompletionQueue();

		// Finally, assemble the server and start
		server_ptr = builder.BuildAndStart();
		std::cout<<"Server listening on "<<server_addr<<std::endl;

		//Proceed to the server's main loop
		handleRPCs();
	}


};

int main(int argc, char **argv)
{
	/* Run the server */
	ServerImpl server;
	server.run();

	return 0;
}

