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

    f.get!(:sub3, :subsub1, :subsub_payload).should == "sub3subsubpayload"
    f.get!(:i3).should == 4
    proc { f.get!(:i2) }.should raise_error(ArgumentError)
    proc { f.get!(:sub2) }.should raise_error(ArgumentError)
  end
end