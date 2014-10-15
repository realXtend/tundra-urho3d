/**
    For conditions of distribution and use, see copyright notice in LICENSE
 
    @file   FrameAPI.h
    @brief  Frame core API. Exposes framework's update tick. */

#pragma once

#include "TundraCoreApi.h"
#include "CoreTypes.h"
#include "Signals.h"

#include <Object.h>
#include <Timer.h>
#include <List.h>

namespace Tundra
{

class Framework;

class DelayedSignal : public Signal1<float>
{
public:
    /// The wallclock time at which was recorded.
    float startTime;
    /// The wallclock time in seconds at which will be triggered.
    float triggerTime;
};

/// Provides a mechanism for plugins and scripts to receive per-frame and time-based events.
/** This class cannot be created directly, it's created by Framework.
    FrameAPI object can be used to:
    -retrieve signal every time frame has been processed
    -retrieve the wall clock time of Framework
    -trigger delayed signals when spesified amount of time has elapsed. */
class TUNDRACORE_API FrameAPI : public Object
{
    OBJECT(FrameAPI);

    /// Return wall clock time of Framework in seconds.
    float WallClockTime() const;

    /// Returns a signal which will be triggered after the specified amount of time has elapsed.
    /*  @param time Time in seconds.
        @note Do not store the signal reference, rather connect to it immediately as necessary */
    DelayedSignal& DelayedExecute(float time);

    /// Returns the current application frame number.
    /** @note It is best not to tie any timing-specific animation to this number, but instead use WallClockTime(). */
    int FrameNumber() const { return currentFrameNumber; }

    /// Emitted when it is time for client code to update their applications.
    /** Scripts and client C++ code can hook into this signal to perform custom per-frame processing.
        This signal is typically used to perform *logic* updates for e.g. game state, networking and other processing.
        @param frametime Elapsed time in seconds since the last frame. */
    Signal1<float> Updated;

    /// Emitted after all frame updates have been processed.
    /** Scripts and client C++ code can hook into this signal to perform custom per-frame processing *after*
        all logic-/state-related updates have been performed. This signal is invoked after the Updated() signal
        has been invoked for all scripts. It allows users to perform processing which depend on the ordering
        of updates of different system. This signal is typically used to perform rendering-related updates for a frame
        that has already been processed to updated state during this frame. 
        @param frameTime Elapsed time in seconds since the last frame. This value has the same value as in the 
            call to the Updated(frametime) signal above. */
    Signal1<float> PostFrameUpdate;

private:
    friend class Framework;

    /// Constructor. Framework takes ownership of this object.
    /** @param fw Framework */
    explicit FrameAPI(Framework *fw);
    ~FrameAPI();

    /// Clears all registered signals to this API.
    void Reset();

    /// Emits Updated signal. Called by Framework each frame. Delayed signals are also processed here.
    /** @param frametime Time elapsed since last frame. */
    void Update(float frametime);

    // Wallclock high-res timer
    mutable Urho3D::HiresTimer wallClock;
    int currentFrameNumber;
    List<DelayedSignal> delayedSignals;
};

}
