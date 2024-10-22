// © 2023, CGVR (https://cgvr.informatik.uni-bremen.de/),
// Author: Gabriel Zachmann, Andre Mühlenbrock (muehlenb@uni-bremen.de)
//
// This is a ported version of the C++ normal estimation implemented by
// Gabriel Zachmann.


#version 330 core

#define EIG_DIM 3
#define EIG_DIM_SQR 9

float SQRT3 = sqrt(3.0);

in vec2 vScreenPos;

uniform float p_h = 0.05f;
uniform float kernelSpread = 1.f;
uniform int kernelRadius = 2;

uniform int depthImageWidth;
uniform int depthImageHeight;

uniform sampler2D texture2D_inputVertices;
uniform sampler2D texture2D_mlsVertices;
uniform sampler2D texture2D_edgeProximity;

out vec4 FragColor;

// Get mat3 element by single index (row-wise):
float getMatrixElem(in mat3 a, in int n){
    return a[n/EIG_DIM][n%EIG_DIM];
}

// Set mat3 element by single index (row-wise):
void setMatrixElem(in out mat3 a, in int n, in float val){
    a[n/EIG_DIM][n%EIG_DIM] = val;
}


void swap(in out int a, in out int b){
    int h = a;
    a = b;
    b = h;
}

// Swaps values of a mat3:
void swap(in out mat3 a, in int i, in int h){
    float i_val = a[i/EIG_DIM][i%EIG_DIM];
    a[i/EIG_DIM][i%EIG_DIM] = a[h/EIG_DIM][h%EIG_DIM];
    a[h/EIG_DIM][h%EIG_DIM] = i_val;
}

// DSWAP for mat3:
void dswap_ported(in out mat3 a, in int x_idx, in int y_idx, in int n )
{
    for ( int i = 0; i < n ; i++ ){
        float x_val = getMatrixElem(a, x_idx + i);
        float y_val = getMatrixElem(a, y_idx + i);

        setMatrixElem(a, x_idx + i, y_val);
        setMatrixElem(a, y_idx + i, x_val);
    }
}

/* 
* Modified DAXPY from blas.c [GZ]
* and modified to work with mat3:
*/

void daxpy_ported(in int n, in float a, in float x[EIG_DIM_SQR], in int x_idx, in out mat3 y, in int y_idx )
{
    for ( int i = 0; i < n ; i ++ )
        setMatrixElem(y, y_idx + i, getMatrixElem(y, y_idx + i) + a * x[x_idx + i]);
}

void cholesky_ported(in out mat3 a, out int jpvt[EIG_DIM], out int rank )
{
    float work[EIG_DIM*EIG_DIM];

    rank = EIG_DIM;

    int k;
    for (  k = 0; k < EIG_DIM; k ++ )
            jpvt[k] = k;

    // reduction loop
    int ak_idx = 0;
    for ( k = 0; k < EIG_DIM; k ++ ){
        int akk_idx = ak_idx + k;
        float maxdia = getMatrixElem(a, akk_idx);
        int maxl = k;

        // determine the pivot element
        int all_idx = akk_idx + EIG_DIM + 1;
        for ( int l = k+1; l <= EIG_DIM-1; l ++ ){
            float all_val = getMatrixElem(a, all_idx);
                if ( all_val > maxdia ){
                    maxdia = all_val;
                    maxl = l;
                }
                all_idx += EIG_DIM + 1;
        }

        // quit if the pivot element is not positive
        if ( maxdia <= 1 ){				// TODO: <= Eps * norm(a) [GZ]
            rank = k;
            break;
        }

        if ( k != maxl ){
            // start the pivoting and update jpvt
            dswap_ported(a, ak_idx, maxl * 3 + maxl, k );

            setMatrixElem(a, maxl * 3 + maxl, getMatrixElem(a, akk_idx)); // Hier ggf. nochmal prüfen.
            setMatrixElem(a, akk_idx, maxdia);

            swap( jpvt[maxl], jpvt[k] );
        }

        // reduction step. pivoting is contained across the rows
        work[k] = sqrt( getMatrixElem(a, akk_idx) );
        setMatrixElem(a, akk_idx, work[k]);
        int aj_idx = ak_idx + EIG_DIM;

        for ( int j = k+1; j < EIG_DIM; j ++ ){
            if ( k != maxl ){
                if ( j < maxl ){
                    swap( a, aj_idx + k, maxl*3+j );
                }
                else if ( j != maxl ){
                    swap( a, aj_idx + k, aj_idx + maxl );
                }
            }


            setMatrixElem(a, aj_idx + k, getMatrixElem(a, aj_idx + k) / work[k]);
            work[j] = getMatrixElem(a, aj_idx + k);
            daxpy_ported( j-k, -work[j], work, k + 1, a, aj_idx + k + 1 );
            aj_idx += EIG_DIM;
        }

        ak_idx += EIG_DIM;
    }
} 


void calc_eigenvalues_unopt(in mat3 m, in out vec3 lambda )
{
    float h1 = m[1][1]*m[2][2];
    float h2 = m[1][2]*m[1][2];
    float h3 = m[0][1]*m[0][1];
    float h4 = m[0][2]*m[0][2];

    float a = -(m[0][0] + m[1][1] + m[2][2]);
    float b = m[0][0]*( m[1][1] + m[2][2]) + h1 - (h2+h3+h4);
    float c = m[0][0]*(h2-h1) + h3*m[2][2] - 2*m[0][1]*m[0][2]*m[1][2] + h4*m[1][1];

    float q = (a*a - 3.0*b) / 9.0;
    float r = (2.0*a*a*a - 9.0*a*b + 27.0*c) / 54.0;
    // post-cond.: r^2 < q^3, because the matrix is a covariance matrix
    // (=> positive definite => 3 real eigenvalues)

    float theta = acos( r / sqrt(q*q*q) );

    lambda[0] = -2.0 * sqrt(q) * cos( theta/3.0 ) - a/3.0;
    lambda[1] = -2.0 * sqrt(q) * cos( theta/3.0 + 3.14159265*2.0/3.0 ) - a/3.0;
    lambda[2] = -2.0 * sqrt(q) * cos( theta/3.0 - 3.14159265*2.0/3.0 ) - a/3.0;
}


// optimized, effect is about 1.3x
void calc_eigenvalues_opt(in mat3 m, in out vec3 lambda)
{
    float h1 = m[1][1]*m[2][2];
    float h2 = m[1][2]*m[1][2];
    float h3 = m[0][1]*m[0][1];
    float h4 = m[0][2]*m[0][2];

    float a = - (m[0][0] + m[1][1] + m[2][2]);
    float b = m[0][0]*( m[1][1] + m[2][2]) + h1 - (h2+h3+h4);
    float c = m[0][0]*(h2-h1) + h3*m[2][2] - 2*m[0][1]*m[0][2]*m[1][2] + h4*m[1][1];

    float q = (a*a - 3.0*b) / 9.0;
    float r = (2.0*a*a*a - 9.0*a*b + 27.0*c) / 54.0;
    // post-cond.: r^2 < q^3, because the matrix is a covariance matrix
    // (=> 3 positive definite => real eigenvalues)

    q = sqrt(q);
    float theta = acos(r / (q*q*q)) / 3.0f;

    float sinth, costh;
    sinth = sin( theta );
    costh = cos( theta );

    a /= 3.0;

    lambda[0] = -2.0 * q * costh - a;
    lambda[1] = q * (costh + sinth*SQRT3) - a;
    lambda[2] = q * (costh - sinth*SQRT3) - a;
}


int calc_eigenvector(mat3 m, in float lambda, out vec3 v )
{
    int i;
    for (i = 0; i < EIG_DIM; i ++ )
        m[i][i] -= lambda;

    int jpvt[EIG_DIM];
    int rank;

    cholesky_ported( m, jpvt, rank );

    // We expect the matrix to have rank=1, so the last row of m
    // is 0, i.e., y is the free variable (before unscrambling!)
    // Forward substitution (solving Ly = 0) yields y=(0,..,0,*),
    // so we can skip that step and start with backward subst.

    float vv[EIG_DIM];
    vv[EIG_DIM-1] = 1.0;
    for ( int k = EIG_DIM-1-1; k >= 0; k -- ){
        vv[k] = 0.0;
        for ( int j = k+1; j < EIG_DIM; j ++ )
            vv[k] -= m[j][k] * vv[j];
        vv[k] /= m[k][k];
    }

    // normalize
    float l = 0.0;

    for ( i = 0; i < EIG_DIM; i ++ )
        l += vv[i] * vv[i];

    l = sqrt( l );

    for ( i = 0; i < EIG_DIM; i ++ )
        vv[i] /= l;

    // unscramble solution
    for ( i = 0; i < EIG_DIM; i ++ )
        v[ jpvt[i] ] = vv[i];

    return rank;
}

/**
 * Represents the weight function of the implicit surface:
 */
 
float calculateTheta(vec3 p, vec3 x){
    float d = distance(p, x);

    return pow(2.71828, -(d*d) / (p_h*p_h));
}

/**
 * Pre-calculates the a(x) value for each depth pixel.
 */

void main()
{
    // Relative size of one pixel:
    vec2 texelSize = kernelSpread / textureSize(texture2D_mlsVertices, 0);

    vec3 a = texture(texture2D_mlsVertices, vScreenPos).xyz;

    // Calculate Covariance Matrix:
    mat3 B = mat3(0);

    int radius = 5;

    for(int dX = -radius; dX <= radius; ++dX){
        for(int dY = -radius; dY <= radius; ++dY){
            vec2 coord = vScreenPos + vec2(dX, dY) * texelSize;

            vec3 p = texture(texture2D_inputVertices, coord).xyz;

            if(isnan(p.x) || isnan(p.y) || isnan(p.z))
                continue;

            float theta = calculateTheta(a, p);

            for(int i = 0; i < 3; ++i){
                for(int j = 0; j < 3; ++j){
                    B[j][i] += theta * (p[i] - a[i]) * (p[j] - a[j]);
                }
            }
        }
    }

    vec3 eigenvalues = vec3(0);
    calc_eigenvalues_opt(B, eigenvalues);

    // Can these eigenvalues be negative? The can't, do they?
    float lambda = min(min(eigenvalues.x, eigenvalues.y), eigenvalues.z);

    vec3 eigenvector = vec3(0);
    calc_eigenvector(B, lambda, eigenvector);

    vec3 normal = -normalize(eigenvector);

    // if normal shows away from the camera, invert it:
    if(normal.z < 0)
        normal = -normal;

    FragColor.rgba = vec4(normal, 1.0);

    if(vScreenPos.x > 0.9993)
        FragColor.rgba = vec4(1.0, 1.0, 1.0, 1.0);
}
