#include "YakudoDouga.h"

typedef struct {
    A_u_long index;
    A_char str[256];
} TableString;

TableString g_strs[StrID_NUMTYPES] = {
    StrID_NONE, "",
    StrID_Name, "YakudoDouga",
    StrID_Description, "Motion-reactive radial blur that grows stronger toward the edges.",
    StrID_Center_Param_Name, "Center",
    StrID_Blur_Strength_Param_Name, "Blur Strength",
    StrID_Motion_Multiplier_Param_Name, "Motion Multiplier",
    StrID_Motion_Frame_Count_Param_Name, "Motion Frames",
    StrID_Blur_Type_Param_Name, "Blur Type",
    StrID_Blur_Type_Choices, "Linear Average|Gaussian|Outward|Jittered Gaussian|Multi-pass|Downsampled",
    StrID_Global_Motion_Param_Name, "Global Motion",
};

char *GetStringPtr(int strNum)
{
    return g_strs[strNum].str;
}
