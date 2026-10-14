#include <CubeMapGenLib/CImageSurface.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <sys/wait.h>
#include <unistd.h>
static void CheckAt(bool ok, int line) { if (!ok) { std::fprintf(stderr, "image fixture failed at line %d\n", line); std::abort(); } }
#define Check(condition) CheckAt((condition), __LINE__)
int main() {
    CImageSurface image;
    Check(!image.m_ImgData);
    image.Init(2,2,3);
    std::array<float,16> input{1,2,3,4,5,6,-99,-99,7,8,9,10,11,12,-99,-99};
    std::array<float,16> output; output.fill(-77);
    image.SetImageData(CP_VAL_FLOAT32,3,8*sizeof(float),input.data());
    image.GetImageData(CP_VAL_FLOAT32,3,8*sizeof(float),output.data());
    for(int row=0;row<2;++row) {
        for(int col=0;col<6;++col) Check(input[row*8+col]==output[row*8+col]);
        Check(output[row*8+6]==-77 && output[row*8+7]==-77);
    }
    image.InPlaceVerticalFlip();
    image.GetImageData(CP_VAL_FLOAT32,3,8*sizeof(float),output.data());
    for(int row=0;row<2;++row) for(int channel=0;channel<3;++channel) {
        Check(output[row*8+channel]==input[row*8+3+channel]);
        Check(output[row*8+3+channel]==input[row*8+channel]);
    }
    image.Init(1,1,4); Check(image.m_ImgData && image.m_Width==1 && image.m_NumChannels==4);
    image.Clear(); Check(!image.m_ImgData && !image.m_Width && !image.m_Height && !image.m_NumChannels);
    image.Clear();
    for(bool negative : {false,true}) {
        const auto child=fork(); Check(child>=0);
        if(!child) { CImageSurface bad; bad.Init(negative?-1:INT_MAX,3,3); std::_Exit(91); }
        int status; Check(waitpid(child,&status,0)==child);
        Check(WIFEXITED(status) && WEXITSTATUS(status)==241);
    }
    std::puts("PASS canonical cubemap CPU image pitched float layout/flip/reinit/clear and terminal negative/overflow allocation bounds");
}
