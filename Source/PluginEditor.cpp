#include "PluginEditor.h"

namespace {
void styleSlider(juce::Slider& s)
{
    s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 20);
    s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xffa855f7));
    s.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff342343));
    s.setColour(juce::Slider::thumbColourId, juce::Colour(0xfff8f7ff));
    s.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xfff8f7ff));
    s.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff160d24));
    s.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}
}
SauceAudioProcessorEditor::SauceAudioProcessorEditor(SauceAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p),
      sauceAtt(p.parameters,"sauce",sauce), inputAtt(p.parameters,"input",input),
      outputAtt(p.parameters,"output",output), mixAtt(p.parameters,"mix",mix),
      subAtt(p.parameters,"sub",sub), transientAtt(p.parameters,"transient",transient),
      boostAtt(p.parameters,"boost",boost), osAtt(p.parameters,"oversampling",oversampling),
      enabledAtt(p.parameters,"enabled",enabled), autoGainAtt(p.parameters,"autogain",autoGain)
{
    setSize(720, 570);
    for (auto* s : {&sauce,&input,&output,&mix,&sub,&transient,&boost}) styleSlider(*s);
    sauce.setTextValueSuffix("%");
    for (auto* s : {&input,&output}) s->setTextValueSuffix(" dB");
    mix.setTextValueSuffix("%"); sub.setTextValueSuffix("%"); transient.setTextValueSuffix("%");
    boost.setTextValueSuffix(" dB");
    for (auto* c : {&preset,&oversampling}) {
        c->setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff160d24));
        c->setColour(juce::ComboBox::textColourId, juce::Colour(0xfff8f7ff));
        c->setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff6d28a8));
        addAndMakeVisible(*c);
    }
    for (int i=0;i<8;++i) preset.addItem(processor.getProgramName(i),i+1);
    preset.setSelectedId(1);
    preset.onChange = [this] { processor.setPreset(preset.getSelectedId()-1); };
    oversampling.addItemList({"1x","2x","4x","8x"},1);
    for (auto* b : {&advanced,&enabled,&autoGain,&prev,&next,&abButton}) {
        b->setColour(juce::TextButton::buttonColourId,juce::Colour(0xff281638));
        b->setColour(juce::TextButton::textColourOffId,juce::Colour(0xfff8f7ff));
        addAndMakeVisible(*b);
    }
    advanced.onClick=[this]{showAdvanced=!showAdvanced; resized();};
    prev.onClick=[this]{int id=juce::jmax(1,preset.getSelectedId()-1);preset.setSelectedId(id);};
    next.onClick=[this]{int id=juce::jmin(8,preset.getSelectedId()+1);preset.setSelectedId(id);};\n    abButton.onClick=[this]{processor.toggleAB(); abButton.setButtonText(processor.isB() ? "B" : "A/B");};
    for (auto* c : {&title,&percent,&inputLabel,&outputLabel}) {
        c->setColour(juce::Label::textColourId,juce::Colour(0xfff8f7ff));
        addAndMakeVisible(*c);
    }
    title.setText("SAUCE  /  prod.shiny",juce::dontSendNotification);
    title.setFont(juce::Font(22.0f,juce::Font::bold));
    percent.setJustificationType(juce::Justification::centred);
    percent.setFont(juce::Font(18.0f,juce::Font::bold));
    inputLabel.setText("INPUT  —",juce::dontSendNotification);
    outputLabel.setText("OUTPUT —",juce::dontSendNotification);
    for (auto* s : {&sauce,&input,&output,&mix,&sub,&transient,&boost}) addAndMakeVisible(*s);
    startTimerHz(30);
}
void SauceAudioProcessorEditor::drawCup(juce::Graphics& g, juce::Rectangle<float> r)
{
    const float level = 0.28f + 0.58f * sauce.getValue()/100.0f;
    auto cup = r.reduced(r.getWidth()*0.18f, r.getHeight()*0.08f);
    juce::Path body; body.startNewSubPath(cup.getX()+cup.getWidth()*0.12f,cup.getY());
    body.lineTo(cup.getRight()-cup.getWidth()*0.12f,cup.getY());
    body.lineTo(cup.getRight()-cup.getWidth()*0.28f,cup.getBottom());
    body.lineTo(cup.getX()+cup.getWidth()*0.28f,cup.getBottom()); body.closeSubPath();
    g.setColour(juce::Colour(0x337c3aed)); g.fillPath(body);
    g.saveState(); g.reduceClipRegion(body);
    auto liquid=juce::Rectangle<float>(cup.getX(),cup.getBottom()-cup.getHeight()*level,cup.getWidth(),cup.getHeight()*level);
    g.setColour(juce::Colour(0xff7c3aed)); g.fillRect(liquid);
    g.setColour(juce::Colour(0xffc4b5fd).withAlpha(0.65f));
    for(int i=0;i<3;++i) {
        float x=cup.getX()+cup.getWidth()*(0.28f+i*0.2f);
        juce::Rectangle<float> ice(x,liquid.getY()+12.0f+(i%2)*9.0f,15,15);
        g.fillRoundedRectangle(ice,3.0f);
    }
    g.restoreState();
    g.setColour(juce::Colour(0xffc4b5fd).withAlpha(0.75f)); g.strokePath(body,juce::PathStrokeType(2.0f));
    g.drawLine(cup.getX()+cup.getWidth()*0.08f,cup.getY()-4,cup.getRight()-cup.getWidth()*0.08f,cup.getY()-4,3.0f);
}
void SauceAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff08070c));
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff21102f),0,0,juce::Colour(0xff08070c),720,570,false));
    g.fillRect(getLocalBounds());
    g.setColour(juce::Colour(0xffa855f7)); g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(8),18,1.2f);
    g.setColour(juce::Colour(0xfff8f7ff)); g.setFont(juce::Font(13.0f,juce::Font::bold));
    g.drawText("LEAN WITH ICE",getLocalBounds().removeFromBottom(24),juce::Justification::centred);
    drawCup(g,{getWidth()-145.0f,125.0f,100.0f,145.0f});
    g.setColour(juce::Colour(0xffa855f7).withAlpha(0.25f));
    g.drawEllipse(205,115,310,310,4.0f);
    g.setColour(juce::Colour(0xffc4b5fd));
    g.drawText("SAUCE INTENSITY",220,405,280,20,juce::Justification::centred);
    const float ip=processor.getInputPeak(), op=processor.getOutputPeak();
    g.setColour(juce::Colour(0xff342343)); g.fillRoundedRectangle(40,500,260,8,4);
    g.fillRoundedRectangle(420,500,260,8,4);
    g.setColour(juce::Colour(0xffa855f7)); g.fillRoundedRectangle(40,500,260*juce::jlimit(0.0f,1.0f,ip),8,4);
    g.setColour(juce::Colour(0xffc4b5fd)); g.fillRoundedRectangle(420,500,260*juce::jlimit(0.0f,1.0f,op),8,4);
}
void SauceAudioProcessorEditor::resized()
{
    auto area=getLocalBounds().reduced(24);
    title.setBounds(28,18,260,34); prev.setBounds(430,20,34,28); preset.setBounds(468,20,180,28); next.setBounds(652,20,34,28);
    advanced.setBounds(28,62,115,28); enabled.setBounds(150,62,70,28); abButton.setBounds(230,62,65,28);
    sauce.setBounds(220,105,280,280); percent.setBounds(280,360,160,30);
    inputLabel.setBounds(40,468,120,22); outputLabel.setBounds(420,468,140,22);
    if(showAdvanced) {
        input.setBounds(24,535,105,90); output.setBounds(135,535,105,90);
        mix.setBounds(246,535,105,90); sub.setBounds(357,535,105,90);
        transient.setBounds(468,535,105,90); boost.setBounds(579,535,105,90);
        autoGain.setBounds(24,625,120,26); oversampling.setBounds(150,625,100,26);
    }
    else {
        for(auto* c : {static_cast<juce::Component*>(&input),&output,&mix,&sub,&transient,&boost,&autoGain,&oversampling}) c->setBounds(0,0,0,0);
    }
}
void SauceAudioProcessorEditor::timerCallback()
{
    percent.setText(juce::String((int)std::round(sauce.getValue()))+"%",juce::dontSendNotification);
    repaint();
}
