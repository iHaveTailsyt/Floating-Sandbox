/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-09-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "UndoStack.h"

#include "Controller.h"

#include <GameCore/Log.h>

namespace ShipBuilder {

template<typename TLayer>
void LayerRegionUndoAction<TLayer>::ApplyAndConsume(Controller & controller)
{
    controller.RestoreLayerRegion(std::move(mLayerRegion), mOrigin);
}

template<typename TLayer>
void WholeLayerUndoAction<TLayer>::ApplyAndConsume(Controller & controller)
{
    controller.RestoreLayer(std::move(mLayer));
}

//
// Explicit specializations
//

template class LayerRegionUndoAction<StructuralLayerData>;
template class LayerRegionUndoAction<ElectricalLayerData>;

template class WholeLayerUndoAction<RopesLayerData>;

/////////////////////////////////////////////////////////////////////////////////////

void UndoStack::Push(std::unique_ptr<UndoAction> && undoAction)
{
    // Update total cost
    mTotalCost += undoAction->GetCost();

    // Push undo action
    mStack.push_back(std::move(undoAction));

    // Trim stack if too big
    while (mStack.size() > MaxEntries || mTotalCost > MaxCost)
    {
        assert(mTotalCost >= mStack.front()->GetCost());
        mTotalCost -= mStack.front()->GetCost();
        mStack.pop_front();
    }
}

std::unique_ptr<UndoAction> UndoStack::Pop()
{
    assert(!mStack.empty());

    // Extract undo action
    auto undoAction = std::move(mStack.back());
    mStack.pop_back();

    // Update total cost
    assert(mTotalCost >= undoAction->GetCost());
    mTotalCost -= undoAction->GetCost();

    return undoAction;
}

}