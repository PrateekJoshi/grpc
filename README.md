# gRPC

## gRPC Docker build and run environment

1. Create the Docker image from `grpc.env.Dockerfile`

2. Mount the gRPC code repo inside the docker conatiner, compile the sources and run

```
$> docker run -it -v /home/prateek/Workspace/grpc/grpc_learn:/app --network host --rm grpc:v1.34.0

#> cd /app
#> make
#> ./pingpong_async_server

```

## gRPC videos

## gRPC talks

## gRPC articles

## References

[API reference](https://grpc.github.io/grpc/cpp/)

[gRPC C++ examples](https://github.com/grpc/grpc/tree/v1.35.0/examples/cpp)
