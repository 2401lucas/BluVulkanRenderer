#ifndef BLUR
    #define BLUR 1

    uniform sampler2D uTexInput;

    uniform int uBlurSize = 4; // use size of noise texture

    in vec2 vTexcoord; // input from vertex shader

    out float fResult;

void main() {
   vec2 texelSize = 1.0 / vec2(textureSize(uInputTex, 0));
   float result = 0.0;
   vec2 hlim = vec2(float(-uBlurSize) * 0.5 + 0.5);
   for (int i = 0; i < uBlurSize; ++i) {
      for (int j = 0; j < uBlurSize; ++j) {
         vec2 offset = (hlim + vec2(float(x), float(y))) * texelSize;
         result += texture(uTexInput, vTexcoord + offset).r;
      }
   }
 
   fResult = result / float(uBlurSize * uBlurSize);
}






#endif