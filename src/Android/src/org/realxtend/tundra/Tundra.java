// For conditions of distribution and use, see copyright notice in LICENSE

package org.realxtend.tundra;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import android.util.Log;

import org.libsdl.app.SDLActivity;

public class Tundra extends SDLActivity {

    @Override
    protected boolean onLoadLibrary(ArrayList<String> libraryNames) {
        // Only load these 3 hardcoded libs, rest are loaded dynamically at runtime
        libraryNames.clear();
        libraryNames.add(new String("Urho3D"));
        libraryNames.add(new String("TundraCore"));
        libraryNames.add(new String("Tundra"));

        return super.onLoadLibrary(libraryNames);
    }
}
