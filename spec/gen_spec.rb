require 'spec_helper'

describe 'Generated code' do
    after(:each) do
        GC.start(full_mark: true, immediate_sweep: true)
    end

    describe 'the message classes' do
        it 'has been created' do
            expect {
                ::Fastproto::TestProtos::TestMessageOne.new
            }.to_not raise_error
        end

        it 'inherits from the base' do
            expect(::Fastproto::TestProtos::TestMessageOne.new).to be_a(::Fastproto::Message)
        end

        it 'can be assigned to and read from' do
            message = ::Fastproto::TestProtos::TestMessageOne.new
            message.id = 77
            expect(message.id).to eql(77);
        end

        it 'starts with default values' do
            expect(::Fastproto::TestProtos::TestMessageOne.new.id).to eql(0)
        end

        it 'has reference semantics with strings' do
            m = ::Fastproto::TestProtos::TestMessageTwo.new
            m.str_field = "foo bar"
            s = m.str_field
            s << " baz"
            expect(m.str_field).to eql("foo bar baz")
        end

        it 'has reference semantics with arrays' do
            m = ::Fastproto::TestProtos::TestMessageFour.new
            m.numbers = [5, 10, 15]
            m.numbers << 787 << 44
            expect(m.numbers).to eql([5, 10, 15, 787, 44])
        end
    end

    describe 'validate!' do
        describe 'an int32 field' do
            it 'works with a good int' do
                100.times do
                m = ::Fastproto::TestProtos::TestMessageOne.new
                m.id = 999
                GC.start(full_mark: true, immediate_sweep: true)
                end
            end

            it 'throws when out of range' do
                m = ::Fastproto::TestProtos::TestMessageOne.new
                m.id = 2**31 + 2
                expect { m.validate! }.to raise_error(RangeError)
            end

            it 'throws when a float' do
                m = ::Fastproto::TestProtos::TestMessageOne.new
                m.id = 1.1
                expect { m.validate! }.to raise_error(TypeError)
            end

            it 'throws when an array' do
                m = ::Fastproto::TestProtos::TestMessageOne.new
                m.id = [1]
                expect { m.validate! }.to raise_error(TypeError)
            end
        end

        describe 'an int64 field' do
            it 'works with a good int' do
                m = ::Fastproto::TestProtos::TestMessageOne.new
                m.field_64 = 2**63 - 2
                expect { m.validate! }.to_not raise_error
            end

            it 'throws when out of range' do
                m = ::Fastproto::TestProtos::TestMessageOne.new
                m.field_64 = 2**63 + 2
                expect { m.validate! }.to raise_error(RangeError)
            end
        end
    end

    describe 'defaults' do
        it 'defaults integers correctly' do
            m = ::Fastproto::TestProtos::TestMessageOne.new
            expect(m.id).to eql(0)
        end

        it 'defaults doubles correclty' do
            m = ::Fastproto::TestProtos::TestMessageTwo.new
            expect(m.double_field).to eql(0.0)
        end

        it 'defaults strings correctly' do
            m = ::Fastproto::TestProtos::TestMessageTwo.new
            expect(m.str_field).to eql("")
        end

        it 'defaults repeateds correctly' do
            m = ::Fastproto::TestProtos::TestMessageFour.new
            expect(m.numbers).to be_a(Array)
            expect(m.numbers.size).to eql(0)
        end
    end

    describe 'has_field?' do
        it 'starts out false' do
            m = ::Fastproto::TestProtos::TestMessageTwo.new
            expect(m.has_str_field?).to eql(false)
        end

        it 'can be set to true by setting the field' do
            m = ::Fastproto::TestProtos::TestMessageTwo.new
            m.str_field = "ohai"
            expect(m.has_str_field?).to eql(true)
        end

        it 'can be set back to false by nulling the field' do
            m = ::Fastproto::TestProtos::TestMessageTwo.new
            m.str_field = "ohai"
            expect(m.has_str_field?).to eql(true)
            m.str_field = nil
            expect(m.str_field).to eql("")
            expect(m.has_str_field?).to eql(false)
        end
    end

    describe 'serialize_to_string' do
        describe 'an int32 field' do
            it 'serializes properly' do
                m = ::Fastproto::TestProtos::TestMessageOne.new
                m.id = 4096
                # Handy tool: http://yura415.github.io/js-protobuf-encode-decode/
                expect(m.serialize_to_string).to eq("\x08\x80\x20".force_encoding(Encoding::ASCII_8BIT))
            end

            it 'serializes it if it is optional and set' do
                m = ::Fastproto::TestProtos::TestMessageOne.new
                m.id = 4096
                m.field_64 = 0
                # Handy tool: http://yura415.github.io/js-protobuf-encode-decode/
                expect(m.serialize_to_string).to eq("\x08\x80\x20\x10\x00".force_encoding(Encoding::ASCII_8BIT))
            end
        end

        describe 'a double field' do
            it 'serializes properly' do
                m = ::Fastproto::TestProtos::TestMessageTwo.new
                m.double_field = 4096.88
                # Handy tool: http://yura415.github.io/js-protobuf-encode-decode/
                expect(m.serialize_to_string).to eq("\x19\x7B\x14\xAE\x47\xE1\x00\xB0\x40".force_encoding(Encoding::ASCII_8BIT))
            end
        end

        describe 'a bool field' do
            it 'serializes properly' do
                m = ::Fastproto::TestProtos::TestMessageThree.new
                m.flag = true
                # Handy tool: http://yura415.github.io/js-protobuf-encode-decode/
                expect(m.serialize_to_string).to eq("\x08\x01".force_encoding(Encoding::ASCII_8BIT))
            end
        end

        describe 'a string field' do
            it 'serializes properly' do
                m = ::Fastproto::TestProtos::TestMessageTwo.new
                m.str_field = "foo bar"
                expect(m.serialize_to_string).to eql("\x22\x07\x66\x6F\x6F\x20\x62\x61\x72".force_encoding(Encoding::ASCII_8BIT))
            end

            it 'serializes properly when it contains nulls' do
                m = ::Fastproto::TestProtos::TestMessageTwo.new
                m.str_field = "foo\x00bar"
                expect(m.serialize_to_string).to eql("\x22\x07\x66\x6F\x6F\x00\x62\x61\x72".force_encoding(Encoding::ASCII_8BIT))
            end

            it 'works with bytes too' do
                # Works just like a string.
                m = ::Fastproto::TestProtos::TestMessageTwo.new
                m.byte_field = "foo bar"
                expect(m.serialize_to_string).to eql("\x2A\x07\x66\x6F\x6F\x20\x62\x61\x72".force_encoding(Encoding::ASCII_8BIT))
            end
        end

        describe 'repeated ints' do
            it 'serializes properly' do
                m = ::Fastproto::TestProtos::TestMessageFour.new
                m.numbers = [99, 77, 55, 32, 9988]
                expect(m.serialize_to_string).to eql("\x08\x63\x08\x4D\x08\x37\x08\x20\x08\x84\x4E".force_encoding(Encoding::ASCII_8BIT))
            end

            it 'serializes empty properly' do
                m = ::Fastproto::TestProtos::TestMessageFour.new
                expect(m.serialize_to_string).to eql("")
            end
        end
    end

    describe 'parse' do
        describe 'an int32 field' do
            it 'deserializes properly' do
                m = ::Fastproto::TestProtos::TestMessageOne.new
                m.parse("\x08\x80\x20".force_encoding(Encoding::ASCII_8BIT))
                expect(m.id).to eql(4096)
            end

            it 'Deserializes optional fields' do
                m = ::Fastproto::TestProtos::TestMessageOne.new
                m.parse("\x08\x80\x20\x10\x03".force_encoding(Encoding::ASCII_8BIT))
                expect(m.id).to eql(4096)
                expect(m.field_64).to eql(3)
            end
        end

        describe 'a double field' do
            it 'deserializes properly' do
                m = ::Fastproto::TestProtos::TestMessageTwo.new
                m.parse("\x19\x7B\x14\xAE\x47\xE1\x00\xB0\x40".force_encoding(Encoding::ASCII_8BIT))
                expect(m.double_field).to eql(4096.88)
            end
        end

        describe 'a bool field' do
            it 'deserializes properly' do
                m = ::Fastproto::TestProtos::TestMessageThree.new
                m.parse("\x08\x01".force_encoding(Encoding::ASCII_8BIT))
                expect(m.flag).to eql(true)
            end
        end

        describe 'a string field' do
            it 'parses properly' do
                m = ::Fastproto::TestProtos::TestMessageTwo.new
                m.parse("\x22\x07\x66\x6F\x6F\x20\x62\x61\x72".force_encoding(Encoding::ASCII_8BIT))
                expect(m.str_field).to eql("foo bar")
            end

            it 'serializes properly when it contains nulls' do
                m = ::Fastproto::TestProtos::TestMessageTwo.new
                m.parse("\x22\x07\x66\x6F\x6F\x00\x62\x61\x72".force_encoding(Encoding::ASCII_8BIT))
                expect(m.str_field).to eql("foo\x00bar")
            end

            it 'works with bytes too' do
                m = ::Fastproto::TestProtos::TestMessageTwo.new
                m.parse("\x2A\x07\x66\x6F\x6F\x20\x62\x61\x72".force_encoding(Encoding::ASCII_8BIT))
                expect(m.byte_field).to eql("foo bar")
            end
        end
    end
end
