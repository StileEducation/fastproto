all: compiled_protobufs/fastproto_gen.bundle

.PHONY: all

compiled_protobufs/fastproto_gen.bundle: ../compiler/rb_fastproto_compiler $(wildcard protobufs/*.proto)
	mkdir -p compiled_protobufs
	protoc \
		--rb_fastproto_out compiled_protobufs \
		--plugin=protoc-gen-rb_fastproto=../compiler/rb_fastproto_compiler \
		-I protobufs \
		-I /usr/include \
		-I /usr/local/include \
		$(wildcard protobufs/*.proto) \
		$(shell ls /usr/local/include/google/protobuf/descriptor.proto /usr/include/google/protobuf/descriptor.proto)
	protoc \
		--cpp_out compiled_protobufs \
		--plugin=protoc-gen-rb_fastproto=../compiler/rb_fastproto_compiler \
		-I protobufs \
		-I /usr/include \
		-I /usr/local/include \
		$(wildcard protobufs/*.proto)
