require 'spec_helper'

describe 'Generated code' do
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
end
