require_relative 'fastproto_compiled_protobufs/fastproto_gen'
require 'securerandom'
STRINGS = (1...1000000).map {  SecureRandom.hex(32) }

sleep 5

i = 0
total_size = 0
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
