all: compiled_protobufs/fastproto_gen.bundle

.PHONY: all

compiled_protobufs/fastproto_gen.bundle: ../compiler/rb_fastproto_compiler $(wildcard protobufs/*.proto)
	mkdir -p compiled_protobufs
	protoc \
		--cpp_out compiled_protobufs \
		--rb_fastproto_out compiled_protobufs \
    --plugin=protoc-gen-rb_fastproto=../compiler/rb_fastproto_compiler \
    $(wildcard protobufs/*.proto)
