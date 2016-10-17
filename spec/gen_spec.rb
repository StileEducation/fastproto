require 'spec_helper'

describe 'Generated code' do
    describe 'the message classes' do
        it 'has been created' do
            expect {
                ::Fastproto::Test::TestMessageOne.new
            }.to_not raise_error
        end
    end
end
