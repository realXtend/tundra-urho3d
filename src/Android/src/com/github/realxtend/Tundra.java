// For conditions of distribution and use, see copyright notice in LICENSE

package com.github.realxtend;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import android.util.Log;

import org.libsdl.app.SDLActivity;

public class Tundra extends SDLActivity {

    @Override
    protected boolean onLoadLibrary(ArrayList<String> libraryNames) {
        // Ensure "Urho3D" and "TundraCore" are at the top of list
        Collections.sort(libraryNames, new Comparator<String>() {
            private String sortName(String name) {
                return name.startsWith("Urho3D") ? "00_" + name : name;
            }

            @Override
            public int compare(String lhs, String rhs) {
                if (lhs.startsWith("Urho3D"))
                    return -1;
                if (rhs.startsWith("Urho3D"))
                    return 1;
                if (lhs.startsWith("TundraCore"))
                    return -1;
                if (rhs.startsWith("TundraCore"))
                    return 1;
                return sortName(lhs).compareTo(sortName(rhs));
            }
        });

        return super.onLoadLibrary(libraryNames);
    }
}
