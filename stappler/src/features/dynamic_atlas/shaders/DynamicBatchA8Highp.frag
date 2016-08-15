
const char* DynamicBatchA8Highp_frag = STRINGIFY(

\n#ifdef GL_ES\n
precision highp float;
\n#endif\n

varying vec4 v_fragmentColor;
varying vec2 v_texCoord;

void main()
{
    gl_FragColor = vec4( v_fragmentColor.rgb,// RGB from uniform\n
        v_fragmentColor.a * texture2D(CC_Texture0, v_texCoord).a // A from texture & uniform\n
    );
}
);
