/** For conditions of distribution and use, see copyright notice in LICENSE

    freelookcamera.js - A free look camera script. Upon run, creates necessary components if they don't exist yet. */

// Global application data
var _g =
{
    connected : false,
    rotate :
    {
        sensitivity : 0.3
    },
    move :
    {
        sensitivity : 30.0,
        amount : new float3(0,0,0)
    },
    motion : new float3(0,0,0),

    // Android specific
    isAndroid : framework.platform == "Android"
};

var inputContext;

function OnScriptDestroyed()
{
    if (framework.IsHeadless() || framework.IsExiting())
        return;
    input.UnregisterInputContextRaw("FreeLookCamera");
    renderer.MainCameraChanged.Disconnect(ActiveCameraChanged);
}

// Entry point
if (!framework.IsHeadless())
{
    // Connect to camera changed signal. This disconnects frame updates when camera is not active
    // and connects back when camera is active. This is a optimization not to have any update processing
    // when the camera is not active. Note that for clients this saves 2x fps updates as the empty "TundraServer"
    // scene would think it is active even when the scene is not visible!
    renderer.MainCameraChanged.Connect(ActiveCameraChanged);

    // Create components & setup default position/lookat for the camera
    var camera = me.GetOrCreateLocalComponent("Camera");
    var placeable = me.GetOrCreateLocalComponent("Placeable");
    //var soundListener = me.GetOrCreateLocalComponent("SoundListener");

    // Co-operate with the AvatarApplication: if AvatarCamera already exists, do not activate the FreeLookCamera right now
    var avatarCameraEntity = scene.EntityByName("AvatarCamera");
    if (!avatarCameraEntity)
    {
        camera.SetActive(); // This will connect frame updates for this script.
        //soundListener.active = true;
    }

    // Register input handling
    var inputContext = input.RegisterInputContextRaw("FreeLookCamera", 101);
    inputContext.KeyPressed.Connect(HandleMove);
    inputContext.KeyReleased.Connect(HandleStop);
    inputContext.MouseEventReceived.Connect(HandleMouse);
    // Connect gestures
    /*
    if (!_g.isAndroid)
    {
        if (inputContext.GestureStarted && inputContext.GestureUpdated)
        {
            inputContext.GestureStarted.connect(GestureStarted);
            inputContext.GestureUpdated.connect(GestureUpdated);
        }
    }
    */
}

function IsCameraActive()
{
    return (me.camera == null ? false : me.camera.IsActive());
}

function Update(frametime)
{
    if (!IsCameraActive())
        return;

    // Android movement logic: move forward if more than 1 finger on screen
    if (_g.isAndroid)
        _g.move.amount.z = input.NumTouchPoints() > 1 ? -1 : 0; 

    if (_g.move.amount.x == 0 && _g.move.amount.y == 0 && _g.move.amount.z == 0)
        return;

    _g.motion.x = _g.move.amount.x * _g.move.sensitivity * frametime;
    _g.motion.y = _g.move.amount.y * _g.move.sensitivity * frametime;
    _g.motion.z = _g.move.amount.z * _g.move.sensitivity * frametime;

    _g.motion = me.placeable.Orientation().Mul(_g.motion);
    me.placeable.SetPosition(me.placeable.Position().Add(_g.motion));
}

function HandleMove(e)
{
    if (!IsCameraActive())
        return;

    if (e.keyCode == 87) // W
        _g.move.amount.z = -1;
    else if (e.keyCode == 83) // S
        _g.move.amount.z = 1;
    else if (e.keyCode == 68) // D
        _g.move.amount.x = 1;
    else if (e.keyCode == 65) // A
        _g.move.amount.x = -1;
    else if (e.keyCode == 32) // Space
        _g.move.amount.y = 1;
    else if (e.keyCode == 67) // C
        _g.move.amount.y = -1;
}

function HandleStop(e)
{
    if (!IsCameraActive())
        return;

    if (e.keyCode == 87 && _g.move.amount.z == -1) // W
        _g.move.amount.z = 0;
    else if (e.keyCode == 83 && _g.move.amount.z == 1) // S
        _g.move.amount.z = 0;
    else if (e.keyCode == 68 && _g.move.amount.x == 1) // D
        _g.move.amount.x = 0;
    else if (e.keyCode == 65 && _g.move.amount.x == -1) // A
        _g.move.amount.x = 0;
    else if (e.keyCode == 32 && _g.move.amount.y == 1) // Space
        _g.move.amount.y = 0;
    else if (e.keyCode == 67 && _g.move.amount.y == -1) // C
        _g.move.amount.y = 0;
}

function HandleMouse(e)
{
    if (!IsCameraActive())
        return;

    // RMB pressed
    if (e.eventType == MouseEvent.MousePressed && e.button == MouseEvent.RightButton && input.IsMouseCursorVisible())
        input.SetMouseCursorVisible(false);
    // RMB released
    else if (e.eventType == MouseEvent.MouseReleased && e.button == MouseEvent.RightButton && !input.IsMouseCursorVisible())
        input.SetMouseCursorVisible(true);

    if (e.IsButtonDown(MouseEvent.RightButton) && !input.IsMouseCursorVisible())
    {
        if (e.relativeX != 0)
            HandleMouseLookX(e.relativeX);
        if (e.relativeY != 0)
            HandleMouseLookY(e.relativeY);
    }
}

function HandleMouseLookX(param)
{
    var transform = me.placeable.transform;
    transform.rot.y -= _g.rotate.sensitivity * parseInt(param);
    me.placeable.transform = transform;
}

function HandleMouseLookY(param)
{
    var transform = me.placeable.transform;
    transform.rot.x -= _g.rotate.sensitivity * parseInt(param);
    if (transform.rot.x > 90.0)
        transform.rot.x = 90.0;
    if (transform.rot.x < -90.0)
        transform.rot.x = -90.0;
    me.placeable.transform = transform;
}

/*
function GestureStarted(gestureEvent)
{
        if (!IsCameraActive())
            return;
    
        if (gestureEvent.GestureType() == Qt.TapAndHoldGesture)
        {
            if (motion_z == 0)
                HandleMove("forward");
            else
                HandleStop("forward");
            gestureEvent.Accept();
        }
        else if (gestureEvent.GestureType() == Qt.PanGesture)
        {
            var offset = gestureEvent.Gesture().offset.toPoint();
            HandleMouseLookX(offset.x());
            HandleMouseLookY(offset.y());
            gestureEvent.Accept();
        }
}

function GestureUpdated(gestureEvent)
{
        if (!IsCameraActive())
            return;

        if (gestureEvent.GestureType() == Qt.PanGesture)
        {
            var delta = gestureEvent.Gesture().delta.toPoint();
            HandleMouseLookX(delta.x());
            HandleMouseLookY(delta.y());
            gestureEvent.Accept();
        }
}
*/

function DisconnectApplication()
{
    if (_g.connected)
    {
        // Disconnect frame updates and enabled SoundListener
        frame.Updated.Disconnect(Update);
        //if (me.soundlistener != null)
        //    me.soundlistener.active = false;
        _g.connected = false;

        _g.move.amount.x = 0;
        _g.move.amount.y = 0;
        _g.move.amount.z = 0;
    }
}

function ConnectApplication()
{
    if (!_g.connected)
    {
        // Connect frame updates and enabled SoundListener
        frame.Updated.Connect(Update);
        //if (me.soundlistener != null)
        //    me.soundlistener.active = true;
        _g.connected = true;

        _g.move.amount.x = 0;
        _g.move.amount.y = 0;
        _g.move.amount.z = 0;
    }
}

function ActiveCameraChanged(cameraEnt)
{
    if (cameraEnt == null)
        return;

    if (cameraEnt == me)
        ConnectApplication();
    else
        DisconnectApplication();
}
