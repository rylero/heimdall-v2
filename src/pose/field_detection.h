#pragma once

// Output of pose estimation: a detection projected onto the field plane.
struct FieldDetection {
    int   class_id;
    float x, y;         // field-relative, meters (WPILib coordinate system)
    float confidence;
};
