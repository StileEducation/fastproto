# encoding: binary

describe ::Fastproto, "message" do
  before(:each) do
  end

  it "correctly handles value_for_tag? when fields are set in the constructor" do
    a = Featureful::A.new(
      :i2 => 1,
      :sub2 => Featureful::A::Sub.new(
        :payload => "test_payload"
      )
    )

    a.value_for_tag?(1).should == true
    a.value_for_tag?(5).should == true
  end

  it "correctly handles value_for_tag? when a MessageField is set to the same object in two locations within the same proto and set in the constructor" do
    d = Featureful::D.new(
      :f => [1, 2, 3].map do |num|
        Featureful::F.new(
          :s => "#{num}"
        )
      end
    )
    c = Featureful::C.new(
      :d => d,
      :e => [1].map do |num|
        Featureful::E.new(
          :d => d
        )
      end
    )

    c.value_for_tag?(1).should == true
  end

  it "correctly handles value_for_tag? when a Messagefield is set to the same object in two locations within the same proto and set outside of the constructor" do
    d = Featureful::D.new
    d.f = [1, 2, 3].map do |num|
      Featureful::F.new(
        :s => "#{num}"
      )
    end
    c = Featureful::C.new
    c.d = d
    c.e = [1].map do |num|
      Featureful::E.new(
        :d => d
      )
    end

    c.value_for_tag?(1).should == true
  end

  it "correctly handles value_for_tag? when a field is accessed and then modified and this field is a MessageField with only a repeated field accessed" do
    c = Featureful::C.new
    c_d = c.d
    c_d.f = [1, 2, 3].map do |num|
      Featureful::F.new(
        :s => "#{num}"
      )
    end
    d = Featureful::D.new
    d.f = [1, 2, 3].map do |num|
      Featureful::F.new(
        :s => "#{num}"
      )
    end
    c.e = [1].map do |num|
      Featureful::E.new(
        :d => d
      )
    end

    c.value_for_tag?(1).should == true
  end

  it "correctly handles value_for_tag? when a field is accessed and then modified and this field is a MessageField with a repeated and optional field accessed" do
    c = Featureful::C.new
    c_d = c.d
    c_d.f = [1, 2, 3].map do |num|
      Featureful::F.new(
        :s => "#{num}"
      )
    end
    d = Featureful::D.new
    d.f = [1, 2, 3].map do |num|
      Featureful::F.new(
        :s => "#{num}"
      )
    end
    d.f2 = Featureful::F.new(
      :s => "4"
    )
    c.e = [1].map do |num|
      Featureful::E.new(
        :d => d
      )
    end

    c.value_for_tag?(1).should == true
  end

  it "correctly handles get" do
    f = Featureful::A.new
    f.i3 = 4
    f.sub3.subsub1.subsub_payload = "sub3subsubpayload"

    f.get(:sub3, :subsub1, :subsub_payload).should == "sub3subsubpayload"
    f.get(:i3).should == 4
    f.get(:i2).should == nil
    f.get(:sub2).should == nil
  end

  it "correctly handles get!" do
    f = Featureful::A.new
    f.i3 = 4
    f.sub3.subsub1.subsub_payload = "sub3subsubpayload"

    f.get!(:sub3, 'subsub1', :subsub_payload).should == "sub3subsubpayload"
    f.get!(:i3).should == 4
    proc { f.get!(:i2) }.should raise_error(ArgumentError)
    proc { f.get!(:sub2) }.should raise_error(ArgumentError)
  end

  it "can handle basic operations" do
    msg1 = Simple::Test1.new
    msg1.test_field.should == ""
    msg1.test_field = "zomgkittenz"
    msg1.i1 = [ 4, 5, 6 ]

    msg2 = Simple::Test1.parse(msg1.serialize_to_string)
    msg2.test_field.should == "zomgkittenz"
    msg2.i1.should == [ 4, 5, 6 ]
    msg2.should == msg1

    msg2.i1 = [ 4, 5, 6, 7 ]
    msg2.should_not == msg1
    msg2.i1 = [ 4, 5, 6 ]
    msg2.should == msg1

    msg1.test_field = "different now"
    msg2.should_not == msg1
  end

  it "correctly unsets fields" do
    msg1 = Simple::Test1.new
    msg1.has_test_field?.should == false
    msg1.test_field.should == ""
    msg1.to_s.should == ""

    msg1.test_field = "zomgkittenz"
    msg1.has_test_field?.should == true
    msg1.test_field.should == "zomgkittenz"
    msg1.to_s.should_not == ""

    msg1.test_field = nil
    msg1.has_test_field?.should == false
    msg1.test_field.should == ""
    msg1.to_s.should == ""
  end

  it "doesn't serialize unset fields" do
    msg1 = Simple::Test1.new
    msg1.has_test_field?.should == false
    msg1.test_field.should == ""
    msg1.to_s.should == ""

    msg2 = Simple::Test1.parse(msg1.to_s)
    msg2.has_test_field?.should == false
    msg2.test_field.should == ""
    msg2.to_s.should == ""

    msg1 = Simple::Test1.new
    msg1.has_test_field?.should == false
    msg1.test_field.should == ""
    msg1.to_s.should == ""

    msg1.test_field = "zomgkittenz"
    msg1.to_s.should_not == ""

    msg1.test_field = nil

    msg2 = Simple::Test1.parse(msg1.to_s)
    msg2.has_test_field?.should == false
    msg2.test_field.should == ""
    msg2.to_s.should == ""
  end

  it "flags values that have been set" do
    a1 = Featureful::A.new
    a1.has_i2?.should == false
    a1.i2 = 5
    a1.has_i2?.should == true
  end

  xit "flags sub-messages that have been set" do
    a1 = Featureful::A.new
    a1.value_for_tag?(a1.class.field_for_name(:sub1).tag).should == true
    a1.value_for_tag?(a1.class.field_for_name(:sub2).tag).should == false
    a1.value_for_tag?(a1.class.field_for_name(:sub3).tag).should == false

    a1.has_sub1?.should == true
    a1.has_sub2?.should == false
    a1.has_sub3?.should == false

    a1.sub2 = Featureful::A::Sub.new(:payload => "ohai")
    a1.has_sub2?.should == true
  end

  xit "flags group that have been set" do
    a1 = Featureful::A.new
    a1.value_for_tag?(a1.class.field_for_name(:group1).tag).should == true
    a1.value_for_tag?(a1.class.field_for_name(:group2).tag).should == false
    a1.value_for_tag?(a1.class.field_for_name(:group3).tag).should == false

    a1.has_group1?.should == true
    a1.has_group2?.should == false
    a1.has_group3?.should == false

    a1.group2 = Featureful::A::Group2.new(:i1 => 1)
    a1.has_group2?.should == true
  end

  describe "#inspect" do
    it "should leave out un-set fields" do
      b1 = Simple::Bar.new
      b1.inspect.should == "#<Simple::Bar foo=<unset>>"
      b1.foo = Simple::Foo.new
      b1.inspect.should == "#<Simple::Bar foo=#<Simple::Foo>>"
    end
  end

  it "detects changes to a sub-message and flags it as set if it wasn't" do
    a1 = Featureful::A.new
    a1.has_sub2?.should == false
    a1.sub2.payload = "ohai"
    a1.has_sub2?.should == true

    a1.has_group2?.should == false
    a1.group2.i1 = 1
    a1.has_sub2?.should == true
  end

  it "detects changes to a sub-sub-message and flags up the chain" do
    a1 = Featureful::A.new
    a1.sub2.has_subsub1?.should == false
    a1.has_sub2?.should == false
    a1.sub2.subsub1.subsub_payload = "ohai"
    a1.has_sub2?.should == true
    a1.sub2.has_subsub1?.should == true
  end

  it "should find a class by its fully-qualified name" do
    a1 = ::Fastproto::Message.find_by_fully_qualified_name("simple.Test1")
    a1.should == Simple::Test1
  end

  it "should find nil for an invalid fully-qualified name" do
    a1 = ::Fastproto::Message.find_by_fully_qualified_name("simple.Test1xxxxxx")
    a1.should == nil
  end
end