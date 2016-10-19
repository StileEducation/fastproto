#!/usr/bin/env ruby

# This file benchmarks single-threaded performance between ruby-protocol-buffers and fastproto.
# We need some data to shove into our arrays.

require 'securerandom'
require 'benchmark'

puts "Generating some random data...."
BIG_INTS = (1...1000000).map { rand(1...(2**62)) }
# FLOATS = (1...2000000).map { rand * 2**60 }
# NEG_INTS = (1...2000000).map { -rand(1...(2**62)) }
STRINGS = (1...1000000).map {  SecureRandom.hex(32) }
puts "...Ready."


def benchmark!
    Benchmark.bm(20) do |x|
        total_size = 0
        x.report("big_ints") {
            i = 0
            30.times do
                m = ::Fastproto::Benchmark::MessageWithRepeateds.new
                m.message_counter = 1
                first = (i * 100000) % BIG_INTS.size
                last = first + 100000
                m.some_big_ints = if last < BIG_INTS.size
                    BIG_INTS[first...last].to_a
                else
                    BIG_INTS[0...100000].to_a
                end
                total_size += m.serialize_to_string.size
                i += 1
            end
        }
        # puts "(total serialized size: #{total_size} bytes)"
        total_size = 0
        x.report("strings") {
            i = 0
            30.times do
                m = ::Fastproto::Benchmark::MessageWithRepeateds.new
                m.message_counter = 1
                first = (i * 100000) % STRINGS.size
                last = first + 100000
                m.some_strings = if last < STRINGS.size
                    STRINGS[first...last].to_a
                else
                    STRINGS[0...100000].to_a
                end
                total_size += m.serialize_to_string.size
                i += 1
            end
        }
        # puts "(total serialized size: #{total_size} bytes)"
        total_size = 0
        x.report("big_ints (no GVL)") {
            i = 0
            30.times do
                m = ::Fastproto::Benchmark::MessageWithRepeateds.new
                m.message_counter = 1
                first = (i * 100000) % BIG_INTS.size
                last = first + 100000
                m.some_big_ints = if last < BIG_INTS.size
                    BIG_INTS[first...last].to_a
                else
                    BIG_INTS[0...100000].to_a
                end
                total_size += m.serialize_to_string_with_gvl.size
                i += 1
            end
        }
        # puts "(total serialized size: #{total_size} bytes)"
        total_size = 0
        x.report("strings (no GVL)") {
            i = 0
            30.times do
                m = ::Fastproto::Benchmark::MessageWithRepeateds.new
                m.message_counter = 1
                first = (i * 100000) % STRINGS.size
                last = first + 100000
                m.some_strings = if last < STRINGS.size
                    STRINGS[first...last].to_a
                else
                    STRINGS[0...100000].to_a
                end
                total_size += m.serialize_to_string_with_gvl.size
                i += 1
            end
        }
        # puts "(total serialized size: #{total_size} bytes)"
    end
end

puts "RUBY-PROTOCOL-BUFFERS BENCHMARK:"
pid = fork do
    require 'protocol_buffers'
    require_relative 'rubypb_compiled_protobufs/protobufs/benchmark.pb'
    class ::ProtocolBuffers::Message
        def serialize_to_string_with_gvl(*args)
            serialize_to_string(*args)
        end
    end
    benchmark!
end
Process.waitpid2 pid

puts "FASTPROTO BENCHMARK:"
pid = fork do
    require_relative 'fastproto_compiled_protobufs/fastproto_gen'
    benchmark!
end
Process.waitpid2 pid
