#version 330

in	vec2	  v_TexCoord;	// varying variable for passing texture coordinate from vertex shader

uniform int         u_Wsize_T;	// temp image width
uniform int         u_Hsize_T;	// temp image height
uniform highp float	u_StepX;
uniform highp float	u_StepY;
uniform highp float	u_StepX_T;
uniform highp float	u_StepY_T;
uniform	sampler2D   u_Sampler;        // source image
uniform	sampler2D   u_Sampler_kernel; // temp image
uniform highp float u_sum_kernel;     // sum of temp image

// uniform variable is the bridge that connect CPU to GPU, you use it to pass data from CPU to GPU


void main()
{
	vec4 image = vec4(0.0);
    vec4 sum = vec4(0.0);
    vec4 convolve = vec4(0.0);
    vec2 tc  = v_TexCoord;
    int  w2  = u_Wsize_T ;
    int  h2  = u_Hsize_T ;
    
    for(int i=0; i<h2; ++i){
       for(int j=0; j<w2; ++j){
            image = texture2D(u_Sampler,vec2(tc.x + j*u_StepX, tc.y + i*u_StepY));
            convolve += image*texture2D(u_Sampler_kernel,vec2(0.0 + j*u_StepX_T, 0.0 + i*u_StepY_T));
            sum += image*image;
         
        }
    }
	
    vec4 result = (convolve/sqrt(sum)/u_sum_kernel);
    gl_FragColor = vec4(result.r, result.r, result.r, 1.0);

}







