

all: $(PROTO_OBJS) $(PROTO_PROTO)

%.pb.cc: %.proto
	protoc -I=$(PROTO_DIR) --cpp_out=$(PROTO_DIR) $(PROTO_DIR)/$<

%.o: %.pb.cc
	$(CC) $(FLAGS) -std=c++11 -c $< -o $@ 

