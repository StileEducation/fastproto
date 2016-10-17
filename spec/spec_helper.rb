require 'rubygems'
require 'bundler/setup'
require_relative 'compiled_protobufs/fastproto_gen'

RSpec.configure do |config|
    config.mock_framework = :rspec
    config.color = true
    config.formatter = 'documentation'
end
