/*==============================================================================

  Library: MSVTK

  Copyright (c) SCS s.r.l. (B3C)

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// VTK includes
#include "vtkButtonRepresentation.h"
#include "vtkButtonWidget.h"
#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSliderWidget.h"
#include "vtkTexturedButtonRepresentation2D.h"
#include "vtkWindowToImageFilter.h"

// MSV includes
#include "msvVTKAnimate.h"
#include "msvVTKButtons.h"
#include "msvVTKButtonsGroup.h"
#include "msvVTKSliderFixedRepresentation2D.h"

vtkStandardNewMacro(msvVTKButtonsGroup);

//-----------------------------------------------------------------------------
// Callback respondign to vtkCommand::StateChangedEvent
class vtkButtonCallbackGroup : public vtkCommand
{
public:
  vtkTypeMacro(vtkButtonCallbackGroup,vtkCommand);

  static vtkButtonCallbackGroup *New()
  {
    return new vtkButtonCallbackGroup;
  }


  virtual void Execute(vtkObject *caller, unsigned long, void*)
  {
    this->State = !State;
    // Show / Hide slider
    this->ToolButton->ShowSlider(State);
    double bounds[6];
    this->ToolButton->GetCameraPoistionOnPath(0,bounds);
    msvVTKAnimate* animateCamera = new msvVTKAnimate();
    animateCamera->Execute(this->Renderer, bounds, 20);
  }

  vtkButtonCallbackGroup() : ToolButton(NULL) , State (false), Renderer(0) {}

  msvVTKButtonsGroup *ToolButton;
  bool State;
  vtkRenderer *Renderer;
};

//-----------------------------------------------------------------------------
// Callback respondign to vtkCommand::HighlightEvent
class vtkButtonHighLightCallbackGroup : public vtkCommand
{
public:
  vtkTypeMacro(vtkButtonHighLightCallbackGroup, vtkCommand)
  static vtkButtonHighLightCallbackGroup *New()
  {
    return new vtkButtonHighLightCallbackGroup;
  }

  virtual void Execute(vtkObject *caller, unsigned long, void*)
  {
    vtkTexturedButtonRepresentation2D *rep =
      reinterpret_cast<vtkTexturedButtonRepresentation2D*>(caller);
    int highlightState = rep->GetHighlightState();

    if ( highlightState == vtkButtonRepresentation::HighlightHovering
      && PreviousHighlightState == vtkButtonRepresentation::HighlightNormal )
    {
      //show tooltip (not if previous state was selecting
      //this->ToolButton->SetShowTooltip(true);
    }
    else if ( highlightState == vtkButtonRepresentation::HighlightNormal)
    {
      //hide tooltip
      //ToolButton->SetShowTooltip(false);
    }
    this->PreviousHighlightState = highlightState;
  }

  vtkButtonHighLightCallbackGroup()
    : ToolButton(NULL), PreviousHighlightState(0) {}
  msvVTKButtonsGroup *ToolButton;
  int PreviousHighlightState;

};

//-----------------------------------------------------------------------------
// Callback respondign to vtkCommand::Interaction
class vtkSliderInteractionCallback : public vtkCommand
{
public:
  vtkTypeMacro(vtkSliderInteractionCallback, vtkCommand)
  static vtkSliderInteractionCallback *New()
  {
    return new vtkSliderInteractionCallback;
  }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
  {
    vtkSliderWidget *sliderWidget = reinterpret_cast<vtkSliderWidget*>(caller);
    double ratio = reinterpret_cast<vtkSliderRepresentation*>(
      sliderWidget->GetRepresentation())->GetValue();
    this->ToolButton->SetCameraPoistionOnPath(ratio);
  }

  vtkSliderInteractionCallback() : ToolButton(NULL), Renderer(0) {}
  vtkRenderer *Renderer;
  msvVTKButtonsGroup *ToolButton;
};

//-----------------------------------------------------------------------------
// Callback respondign to vtkCommand::StartInteraction
class vtkSliderStartInteractionCallback : public vtkCommand
{
public:
  vtkTypeMacro(vtkSliderStartInteractionCallback, vtkCommand)
  static vtkSliderStartInteractionCallback *New()
  {
    return new vtkSliderStartInteractionCallback;
  }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
  {
    vtkSliderWidget *sliderWidget = reinterpret_cast<vtkSliderWidget*>(caller);
    msvVTKAnimate* animateCamera = new msvVTKAnimate();
    double ratio = reinterpret_cast<vtkSliderRepresentation*>(sliderWidget->GetRepresentation())->GetValue();
    double bounds[6];
    this->ToolButton->GetCameraPoistionOnPath(ratio,bounds);
    animateCamera->Execute(this->Renderer, bounds, 20);
  }

  vtkSliderStartInteractionCallback() : ToolButton(NULL), Renderer(0) {}
  vtkRenderer *Renderer;
  msvVTKButtonsGroup *ToolButton;
};

//-----------------------------------------------------------------------------
msvVTKButtonsGroup::msvVTKButtonsGroup() : msvVTKButtonsInterface()
{
  this->SliderWidget = NULL;

  this->ButtonCallback = vtkButtonCallbackGroup::New();
  vtkButtonCallbackGroup::SafeDownCast(
    this->ButtonCallback)->ToolButton = this;

  vtkButtonHighLightCallbackGroup* hc = vtkButtonHighLightCallbackGroup::New();
  hc->ToolButton = this;
  this->HighlightCallback = hc;

  this->SliderInteractionCallback = vtkSliderInteractionCallback::New();
  this->SliderInteractionCallback->ToolButton = this;

  this->SliderStartInteractionCallback = vtkSliderStartInteractionCallback::New();
  this->SliderStartInteractionCallback->ToolButton = this;

  this->GetButton()->AddObserver(
    vtkCommand::StateChangedEvent,this->ButtonCallback);

  this->GetButton()->GetRepresentation()->AddObserver(
    vtkCommand::HighlightEvent,this->HighlightCallback);
}

//-----------------------------------------------------------------------------
msvVTKButtonsGroup::~msvVTKButtonsGroup()
{

}

//-----------------------------------------------------------------------------
void msvVTKButtonsGroup::AddElement(msvVTKButtonsInterface* buttons)
{
  //int i = 0;
  double b[6];
  buttons->GetBounds(b);
  double dimension = (b[1]-b[0])*(b[3]-b[2])*(b[5]-b[4]);

  for(std::vector<msvVTKButtonsInterface*>::iterator buttonsIt = Elements.begin(); buttonsIt != Elements.end(); buttonsIt++)
    {
    if(*buttonsIt == buttons)
      {
      return;
      }
    (*buttonsIt)->GetBounds(b);
    double cur_dimension = (b[1]-b[0])*(b[3]-b[2])*(b[5]-b[4]);
    if(dimension > cur_dimension)
      {
      Elements.insert(buttonsIt,buttons);
      return;
      }
    //i++;
    }
  Elements.push_back(buttons);
}

//-----------------------------------------------------------------------------
void msvVTKButtonsGroup::RemoveElement(msvVTKButtonsInterface* buttons)
{
  //int index = 0;
  for(std::vector<msvVTKButtonsInterface*>::iterator buttonsIt = Elements.begin(); buttonsIt != Elements.end(); buttonsIt++)
    {
    if(*buttonsIt == buttons)
      {
      Elements.erase(buttonsIt);
      return;
      }
    //index++;
    }
}

//-----------------------------------------------------------------------------
msvVTKButtonsInterface* msvVTKButtonsGroup::GetElement(unsigned int index)
{
  if(index > Elements.size() - 1)
    {
    return NULL;
    }
  return Elements.at(index);
}

//-----------------------------------------------------------------------------
msvVTKButtonsGroup* msvVTKButtonsGroup::CreateGroup()
{
  return CreateElement<msvVTKButtonsGroup>();
}

//-----------------------------------------------------------------------------
msvVTKButtons* msvVTKButtonsGroup::CreateButtons()
{
  return CreateElement<msvVTKButtons>();
}

//-----------------------------------------------------------------------------
template <class T>
T *msvVTKButtonsGroup::CreateElement()
{
  T *element = T::New();
  this->AddElement(element);
  return element;
}

//-----------------------------------------------------------------------------
void msvVTKButtonsGroup::SetShowButton(bool show)
{
  for (std::vector<msvVTKButtonsInterface*>::iterator buttonsIt = Elements.begin();
       buttonsIt != Elements.end(); buttonsIt++)
    {
    (*buttonsIt)->SetShowButton(show);
    }
}

//-----------------------------------------------------------------------------
void msvVTKButtonsGroup::SetShowLabel(bool show)
{
  for (std::vector<msvVTKButtonsInterface*>::iterator buttonsIt = Elements.begin();
      buttonsIt != Elements.end(); buttonsIt++)
    {
    (*buttonsIt)->SetShowLabel(show);
    }
}

//-----------------------------------------------------------------------------
void msvVTKButtonsGroup::SetImages(vtkImageData *image)
{
  for (std::vector<msvVTKButtonsInterface*>::iterator buttonsIt = Elements.begin();
       buttonsIt != Elements.end(); buttonsIt++)
    {
    (*buttonsIt)->SetImage(image);
    }
}

//-----------------------------------------------------------------------------
vtkSliderWidget* msvVTKButtonsGroup::GetSlider()
{
  if (!this->SliderWidget)
    {
    msvVTKSliderFixedRepresentation2D* sliderRep =
      msvVTKSliderFixedRepresentation2D::New();

    double translate[2] = {30,70};

    sliderRep->SetMinimumValue(0.0);
    sliderRep->SetMaximumValue(100.0);
    sliderRep->SetLabelFormat("%0.f%%");
    sliderRep->SetValue(0.0);
    sliderRep->SetTitleText("");
    sliderRep->SetTranslate(translate);
    sliderRep->GetPoint1Coordinate()->SetCoordinateSystemToDisplay();
    sliderRep->GetPoint1Coordinate()->SetValue(0,0);
    sliderRep->GetPoint2Coordinate()->SetCoordinateSystemToDisplay();
    sliderRep->GetPoint2Coordinate()->SetValue(0,50);
    sliderRep->SetSliderLength(0.07);
    sliderRep->SetSliderWidth(0.14);
    sliderRep->SetEndCapLength(0.07);
    sliderRep->SetEndCapWidth(0.14);
    sliderRep->SetTubeWidth(0.015);
    sliderRep->GetSliderProperty()->SetColor(.35,.45,.6);
    sliderRep->GetSliderProperty()->SetOpacity(.9);
    sliderRep->GetCapProperty()->SetOpacity(.9);
    sliderRep->GetCapProperty()->SetColor(.3,.4,.5);
    sliderRep->GetSelectedProperty()->SetOpacity(.9);
    sliderRep->GetSelectedProperty()->SetColor(.4,.5,.7);
    sliderRep->GetTubeProperty()->SetOpacity(.9);
    sliderRep->GetTubeProperty()->SetColor(.3,.4,.5);
    sliderRep->SetVisibility(false);
    this->SliderWidget = vtkSliderWidget::New();
    this->SliderWidget->SetRepresentation(sliderRep);
    this->SliderWidget->SetAnimationModeToAnimate();
    sliderRep->Delete();
    this->SliderWidget->AddObserver(vtkCommand::InteractionEvent,
                                    this->GetSliderInteractionCallback());
    this->SliderWidget->AddObserver(vtkCommand::StartInteractionEvent,
                                    this->GetSliderStartInteractionCallback());
    }
  return this->SliderWidget;
}

//-----------------------------------------------------------------------------
void msvVTKButtonsGroup::ShowSlider(bool show)
{
  if(this->GetSlider())
    {
    this->GetSlider()->GetRepresentation()->SetVisibility(show);
    this->GetSlider()->Render();
    }
}

//-----------------------------------------------------------------------------
void msvVTKButtonsGroup::GetCameraPoistionOnPath(double ratio, double b[6])
{
  const size_t numOfElements = this->Elements.size();

  if(numOfElements < 1)
    {
    return;
    }
  double ratioPerElement = 100. / double(numOfElements - 1);
  int targetElement = int(ratio / ratioPerElement);

  double subPathRatio =
    (ratio - (double(targetElement) * ratioPerElement)) / ratioPerElement;
  // (0-1 value)

  // calculate intermediate bounds
  //double resetBounds[6];
  double b1[6];
  double b2[6];

  this->GetElement(targetElement)->GetBounds(b1);
  if(ratio == 0 || targetElement == numOfElements - 1)
    {
    b[0] = b1[0];
    b[1] = b1[1];
    b[2] = b1[2];
    b[3] = b1[3];
    b[4] = b1[4];
    b[5] = b1[5];
    }
  else
    {
    this->GetElement(targetElement + 1)->GetBounds(b2);
    b[0] = b1[0] * (1 - subPathRatio) + b2[0] * subPathRatio;
    b[1] = b1[1] * (1 - subPathRatio) + b2[1] * subPathRatio;
    b[2] = b1[2] * (1 - subPathRatio) + b2[2] * subPathRatio;
    b[3] = b1[3] * (1 - subPathRatio) + b2[3] * subPathRatio;
    b[4] = b1[4] * (1 - subPathRatio) + b2[4] * subPathRatio;
    b[5] = b1[5] * (1 - subPathRatio) + b2[5] * subPathRatio;
    }
}

//-----------------------------------------------------------------------------
void msvVTKButtonsGroup::SetCameraPoistionOnPath(double ratio)
{
  double resetBounds[6];
  GetCameraPoistionOnPath(ratio,resetBounds);
  GetSlider()->GetCurrentRenderer()->ResetCamera(resetBounds);
}

//-----------------------------------------------------------------------------
vtkCommand *msvVTKButtonsGroup::GetSliderInteractionCallback() const
{
  return this->SliderInteractionCallback;
}

//-----------------------------------------------------------------------------
vtkCommand *msvVTKButtonsGroup::GetSliderStartInteractionCallback() const
{
  return this->SliderStartInteractionCallback;
}

//-----------------------------------------------------------------------------
void msvVTKButtonsGroup::SetCurrentRenderer(vtkRenderer *renderer)
{
  this->msvVTKButtonsInterface::SetCurrentRenderer(renderer);
  this->GetSlider()->SetInteractor(renderer ? renderer->GetRenderWindow()->GetInteractor() : NULL);
  this->GetSlider()->SetCurrentRenderer(renderer); //to check
  this->GetSlider()->GetRepresentation()->SetRenderer(renderer);

  vtkSliderInteractionCallback::SafeDownCast(
    this->SliderInteractionCallback)->Renderer = renderer;

  vtkSliderStartInteractionCallback::SafeDownCast(
    this->SliderStartInteractionCallback)->Renderer = renderer;

  vtkButtonCallbackGroup::SafeDownCast(
    this->ButtonCallback)->Renderer = renderer;

  this->GetSlider()->SetEnabled(renderer ? true : false);

  for (std::vector<msvVTKButtonsInterface*>::iterator buttonsIt = Elements.begin();
       buttonsIt != Elements.end(); buttonsIt++)
    {
    (*buttonsIt)->SetCurrentRenderer(renderer);
    }
}

//-----------------------------------------------------------------------------
void msvVTKButtonsGroup::Update()
{
  this->CalculatePosition();
  this->Superclass::Update();
}

//-----------------------------------------------------------------------------
void msvVTKButtonsGroup::CalculatePosition()
{
  double bds[6];

  bds[0] = 0;
  bds[1] = 16;
  bds[2] = 0;
  bds[3] = 16;
  bds[4] = 0;
  bds[5] = 2;

  vtkTexturedButtonRepresentation2D *rep =
    vtkTexturedButtonRepresentation2D::SafeDownCast(
      this->GetButton()->GetRepresentation());
  rep->PlaceWidget(bds);
  rep->Modified();
  this->GetButton()->SetRepresentation(rep);
}