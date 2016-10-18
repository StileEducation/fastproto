require 'spec_helper'

describe 'Generated code' do
    after(:each) do
        GC.start(full_mark: true, immediate_sweep: true)
    end

    describe 'the message classes' do
        it 'has been created' do
            expect {
                ::Fastproto::Test::TestMessageOne.new
            }.to_not raise_error
        end

        it 'inherits from the base' do
            expect(::Fastproto::Test::TestMessageOne.new).to be_a(::Fastproto::Message)
        end

        it 'can be assigned to and read from' do
            message = ::Fastproto::Test::TestMessageOne.new
            message.id = 77
            expect(message.id).to eql(77);
        end

        it 'starts with default values' do
            expect(::Fastproto::Test::TestMessageOne.new.id).to eql(0)
        end
    end

    describe 'validate!' do
        describe 'an int32 field' do
            it 'works with a good int' do
                100.times do
                m = ::Fastproto::Test::TestMessageOne.new
                m.id = 999
                GC.start(full_mark: true, immediate_sweep: true)
                end
            end

            it 'throws when out of range' do
                m = ::Fastproto::Test::TestMessageOne.new
                m.id = 2**31 + 2
                expect { m.validate! }.to raise_error(TypeError)
            end

            it 'throws when a float' do
                m = ::Fastproto::Test::TestMessageOne.new
                m.id = 1.1
                expect { m.validate! }.to raise_error(TypeError)
            end

            it 'throws when an array' do
                m = ::Fastproto::Test::TestMessageOne.new
                m.id = [1]
                expect { m.validate! }.to raise_error(TypeError)
            end
        end

        describe 'an int64 field' do
            it 'works with a good int' do
                m = ::Fastproto::Test::TestMessageOne.new
                m.field_64 = 2**63 - 2
                expect { m.validate! }.to_not raise_error
            end

            it 'throws when out of range' do
                m = ::Fastproto::Test::TestMessageOne.new
                m.field_64 = 2**63 + 2
                expect { m.validate! }.to raise_error(TypeError)
            end
        end
    end

    describe 'serialize_to_string' do
        describe 'an int32 field' do
            it 'serializes properly' do
                m = ::Fastproto::Test::TestMessageOne.new
                m.id = 4096
                # Handy tool: http://yura415.github.io/js-protobuf-encode-decode/
                expect(m.serialize_to_string).to eq("\x08\x80\x20\x10\x00".force_encoding(Encoding::ASCII_8BIT))
            end
        end
    end

    describe 'parse' do
        describe 'an int32 field' do
            it 'deserializes properly' do
                m = ::Fastproto::Test::TestMessageOne.new
                m.parse(StringIO.new("\x08\x80\x20".force_encoding(Encoding::ASCII_8BIT)))
                expect(m.id).to eql(4096)
            end
        end
    end
end
