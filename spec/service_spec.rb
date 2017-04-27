# encoding: binary

describe ::Fastproto, "service" do
  before(:each) do
  end

  it "correctly handles service definitions" do
    get_foo_rpc, get_bar_rpc = get_rpcs

    get_foo_rpc.name.should == :get_foo
    get_foo_rpc.proto_name.should == "GetFoo"
    get_foo_rpc.request_class.should == Services::FooRequest
    get_foo_rpc.response_class.should == Services::FooResponse
    get_foo_rpc.service_class.should == Services::FooBarService

    get_bar_rpc.name.should == :get_bar
    get_bar_rpc.proto_name.should == "GetBar"
    get_bar_rpc.request_class.should == Services::BarRequest
    get_bar_rpc.response_class.should == Services::BarResponse
    get_bar_rpc.service_class.should == Services::FooBarService
  end

  it "correctly handles == for Rpcs" do
    get_foo_rpc, get_bar_rpc = get_rpcs

    get_foo_rpc.should == get_foo_rpc
    get_bar_rpc.should == get_bar_rpc
    get_foo_rpc.should_not == get_bar_rpc
  end

  it "correctly freezes rpcs" do
    get_foo_rpc, get_bar_rpc = get_rpcs

    get_foo_rpc.frozen?.should == true
    get_bar_rpc.frozen?.should == true
    get_foo_rpc.proto_name.frozen?.should == true
    get_bar_rpc.proto_name.frozen?.should == true

    # make sure to_s is still possible when frozen
    get_foo_rpc.to_s
    get_bar_rpc.to_s

    Services::FooBarService.rpcs.frozen?.should == true
  end

  it "correctly handles fully qualified names on Services" do
    Services::FooBarService.fully_qualified_name.should == "services.FooBarService"
  end

  def get_rpcs
    Services::FooBarService.rpcs.size.should == 2

    first_rpc = Services::FooBarService.rpcs[0]
    second_rpc = Services::FooBarService.rpcs[1]

    case first_rpc.name
    when :get_foo
      second_rpc.name.should == :get_bar
      return first_rpc, second_rpc
    when :get_bar
      first_rpc.name.should == :get_bar
      return second_rpc, first_rpc
    else raise ArgumentError.new(first_rpc.name)
    end
  end
end