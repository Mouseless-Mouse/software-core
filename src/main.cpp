#include <Arduino.h>

#include "matrix.h"
#include "display.h"
#include "mtx_asm.h"

#define ATTRACTOR_HALVORSEN

TFT_Parallel display(320, 170);

const float FOV = 90 * PI / 180;
const float FAR_PLANE_Z = 10;
const float NEAR_PLANE_Z = 1;

#if 1

const Array<float, 4, 4> perspective_projection_mtx{
  {1/tan(FOV/2), 0,            0,                                                            0},
  {0,            1/tan(FOV/2), 0,                                                            0},
  {0,            0,            -FAR_PLANE_Z / (FAR_PLANE_Z - NEAR_PLANE_Z),                 -1},
  {0,            0,            -(FAR_PLANE_Z * NEAR_PLANE_Z) / (FAR_PLANE_Z - NEAR_PLANE_Z), 0}
};

struct Particle {
  Vector<float, 4> pos;
  uint16_t color;
  static const Array<float, 4, 4> fixed_transform_mtx;
  static const Array<float, 4, 4> cam_transform_mtx;
  Particle() : pos{{rand()%250/125.f - 1.f}, {rand()%250/125.f - 1.f}, {rand()%250/125.f - 1.f}, {1.f}}, color(rand()&1 ? color_rgb(0, 255, 255) : color_rgb(0, 0, 255)) {}
  void move() {
#if defined(ATTRACTOR_THOMAS)
    float dx = sin(pos[1]*2)-0.19*pos[0]*2;
    float dy = sin(pos[2]*2)-0.19*pos[1]*2;
    float dz = sin(pos[0]*2)-0.19*pos[2]*2;
#elif defined(ATTRACTOR_HALVORSEN)
    float dx = 0.01f * (-1.5f*pos[0]*6 - 4*pos[1]*6 - 4*pos[2]*6 - pow(pos[1]*6, 2.f));
    float dy = 0.01f * (-1.5f*pos[1]*6 - 4*pos[2]*6 - 4*pos[0]*6 - pow(pos[2]*6, 2.f));
    float dz = 0.01f * (-1.5f*pos[2]*6 - 4*pos[0]*6 - 4*pos[1]*6 - pow(pos[0]*6, 2.f));
#endif
    pos[0] += 0.05f*dx;
    pos[1] += 0.05f*dy;
    pos[2] += 0.05f*dz;
    if (pos[2] < -5.f || pos[2] > 5.f) {
      pos[0] = rand()%250/125.f - 1.f;
      pos[1] = rand()%250/125.f - 1.f;
      pos[2] = rand()%250/125.f - 1.f;
    }
  }
  uint32_t draw(const Array<float, 4, 4>& intermediate_transform_mtx) {
    uint32_t mulStart = xthal_get_ccount();
    Vector<float, 4> canvas_coords = cam_transform_mtx * (intermediate_transform_mtx * (fixed_transform_mtx * pos));
    uint32_t mulEnd = xthal_get_ccount();
    display.drawPixel(160 + canvas_coords[0]/canvas_coords[3]*85.f, 85 + canvas_coords[1]/canvas_coords[3]*85.f, color);
    return mulEnd - mulStart;
  }
};

const Array<float, 4, 4> Particle::fixed_transform_mtx = rotX(-57 * PI/180) * rotY(-PI/2) * rotX(135 * PI/180);
const Array<float, 4, 4> Particle::cam_transform_mtx = perspective_projection_mtx * translate(Vector<float, 3>{{0}, {0}, {4}});

#else

const Mat4_fp16 perspective_projection_mtx(
  1/tan(FOV/2), 0,            0,                                                            0,
  0,            1/tan(FOV/2), 0,                                                            0,
  0,            0,            -FAR_PLANE_Z / (FAR_PLANE_Z - NEAR_PLANE_Z),                 -1,
  0,            0,            -(FAR_PLANE_Z * NEAR_PLANE_Z) / (FAR_PLANE_Z - NEAR_PLANE_Z), 0
);

struct Particle {
  Vec4_fp16 pos;
  uint16_t color;
  static const Mat4_fp16 fixed_transform_mtx;
  static const Mat4_fp16 cam_transform_mtx;
  Particle() : pos(rand()%250/125.f - 1.f, rand()%250/125.f - 1.f, rand()%250/125.f - 1.f, 1.f), color(rand()&1 ? color_rgb(0, 255, 255) : color_rgb(0, 0, 255)) {}
  void move() {
#if defined(ATTRACTOR_THOMAS)
    float dx = sin(pos[1]*2)-0.19*pos[0]*2;
    float dy = sin(pos[2]*2)-0.19*pos[1]*2;
    float dz = sin(pos[0]*2)-0.19*pos[2]*2;
#elif defined(ATTRACTOR_HALVORSEN)
    float dx = 0.01f * (-1.5f*((float)pos[0])*6 - 4*((float)pos[1])*6 - 4*((float)pos[2])*6 - pow(((float)pos[1])*6, 2.f));
    float dy = 0.01f * (-1.5f*((float)pos[1])*6 - 4*((float)pos[2])*6 - 4*((float)pos[0])*6 - pow(((float)pos[2])*6, 2.f));
    float dz = 0.01f * (-1.5f*((float)pos[2])*6 - 4*((float)pos[0])*6 - 4*((float)pos[1])*6 - pow(((float)pos[0])*6, 2.f));
#endif
    pos[0] = (float)pos[0] + 0.05f*dx;
    pos[1] = (float)pos[1] + 0.05f*dy;
    pos[2] = (float)pos[2] + 0.05f*dz;
    if (pos[2] < -4.f || pos[2] > 4.f) {
      pos[0] = rand()%250/125.f - 1.f;
      pos[1] = rand()%250/125.f - 1.f;
      pos[2] = rand()%250/125.f - 1.f;
    }
  }
  uint32_t draw(Mat4_fp16& intermediate_transform_mtx) {
    uint32_t mulStart = xthal_get_ccount();
    Vec4_fp16 canvas_coords = cam_transform_mtx * (intermediate_transform_mtx * (fixed_transform_mtx * pos));
    uint32_t mulEnd = xthal_get_ccount();
    display.drawPixel(160 + ((float)canvas_coords[0])/((float)canvas_coords[3])*85.f, 85 + ((float)canvas_coords[1])/((float)canvas_coords[3])*85.f, color);
    return mulEnd - mulStart;
  }
};

const Mat4_fp16 Particle::fixed_transform_mtx = rotX_fp16(-57 * PI/180) * rotY_fp16(-PI/2) * rotX_fp16(135 * PI/180);
const Mat4_fp16 Particle::cam_transform_mtx = perspective_projection_mtx * translate_fp16(0, 0, 4);

#endif

const size_t NUM_PARTICLES = 1000;
Particle *mem;

void setup() {
  // put your setup code here, to run once:
  setCpuFrequencyMhz(240);
  Serial.begin(115200);

  delay(3000);

  Mat4_fp16 myMtx(
    0.1f, 0.2f, 0.3f, 0.4f,
    0.5f, 0.6f, 0.7f, 0.8f,
    0.9f, 1.0f, 1.1f, 1.2f,
    1.3f, 1.4f, 1.5f, 1.6f
  );
  Mat4_fp16 myOtherMtx(
    0, -1, 2, 1,
    -1, 0, -1, 2,
    2, -1, 0, -1,
    1, 2, -1, 0
  );
  myMtx.print();
  Vec4_fp16 *vectsIn = reinterpret_cast<Vec4_fp16*>(heap_caps_aligned_alloc(16, 1000*sizeof(Vec4_fp16), MALLOC_CAP_INTERNAL));

  for (size_t i = 0; i < 1000; ++i)
    new(&(vectsIn[i])) Vec4_fp16(1.f-random()%8192/4096.f, 1.f-random()%8192/4096.f, 1.f-random()%8192/4096.f, 1.f-random()%8192/4096.f);
  volatile Vec4_fp16 *vectsOut = reinterpret_cast<volatile Vec4_fp16*>(heap_caps_aligned_alloc(16, 1000*sizeof(Vec4_fp16), MALLOC_CAP_INTERNAL));

  if (!vectsIn || !vectsOut) {
    Serial.println("Could not allocate vector memory");
    while(1);
  }
  
  Serial.printf("Inputs:\n{%f, %f, %f, %f}\n{%f, %f, %f, %f}\n",
    (float)vectsIn[2][0], (float)vectsIn[2][1], (float)vectsIn[2][2], (float)vectsIn[2][3],
    (float)vectsIn[3][0], (float)vectsIn[3][1], (float)vectsIn[3][2], (float)vectsIn[3][3]
  );

  unsigned long tStart = micros();
  myMtx.mul_vector_list(vectsIn, vectsOut, 1000);
  unsigned long tEnd = micros();

  Serial.printf("Result:\n{%f, %f, %f, %f}\n{%f, %f, %f, %f}\n",
    (float)vectsOut[2][0], (float)vectsOut[2][1], (float)vectsOut[2][2], (float)vectsOut[2][3],
    (float)vectsOut[3][0], (float)vectsOut[3][1], (float)vectsOut[3][2], (float)vectsOut[3][3]
  );

  Serial.printf("1000 vector transforms took %i microseconds\n", tEnd - tStart);

  mul_vector_list_scalar(vectsIn, vectsOut, 3, 1000);
  for (size_t i = 0; i < 1000; ++i) {
    msg_assert(abs(((float)vectsOut[i][0])/((float)vectsIn[i][0]) - 3) < 0.01, "Arithmetic error (%f / %f != 3)", (float)vectsOut[i][0], (float)vectsIn[i][0]);
    msg_assert(abs(((float)vectsOut[i][1])/((float)vectsIn[i][1]) - 3) < 0.01, "Arithmetic error (%f / %f != 3)", (float)vectsOut[i][1], (float)vectsIn[i][1]);
    msg_assert(abs(((float)vectsOut[i][2])/((float)vectsIn[i][2]) - 3) < 0.01, "Arithmetic error (%f / %f != 3)", (float)vectsOut[i][2], (float)vectsIn[i][2]);
    msg_assert(abs(((float)vectsOut[i][3])/((float)vectsIn[i][3]) - 3) < 0.01, "Arithmetic error (%f / %f != 3)", (float)vectsOut[i][3], (float)vectsIn[i][3]);
  }

  heap_caps_free(vectsIn);
  heap_caps_free((void*)vectsOut);
  vectsIn = nullptr;
  vectsOut = nullptr;

  Mat4_fp16 myResultMtx = myMtx * myOtherMtx;
  Serial.println("Matrix multiplication result:");
  myMtx.print();
  Serial.println("X");
  myOtherMtx.print();
  Serial.println("=");
  myResultMtx.print();

  display.init();

  mem = reinterpret_cast<Particle*>(heap_caps_malloc(NUM_PARTICLES * sizeof(Particle), MALLOC_CAP_SPIRAM));
  if (!mem) {
    Serial.println("Big sad");
    while(1);
  }
  
  for (ptrdiff_t i = 0; i < NUM_PARTICLES; ++i) {
    new(mem + i) Particle();
  }

  Serial.println("Entering loop");
  display.setTextColor(color_rgb(255, 255, 255));
  display.setTextSize(2);
}

uint32_t startTime = 0;
void loop() {
  // put your main code here, to run repeatedly:
  static uint32_t t = 0;
  uint32_t mulTime = 0;
  while (!display.done_refreshing());
  display.clear();
  display.setCursor(1, 150);
  display.printf("%f fps", 1000000.f / (micros() - startTime));
  startTime = micros();
  display.setCursor(1, 1);
  display.print("Hello, Mouseless World!");
  Array<float, 4, 4> rotTransform = rotZ(t/120.f);
  for (size_t i = 0; i < NUM_PARTICLES; ++i) {
    mulTime += mem[i].draw(rotTransform);
  }
  ++t;
  display.refresh();
  for (size_t i = 0; i < NUM_PARTICLES; ++i) {
    mem[i].move();
  }
  if (!(t&255)) {
    Serial.printf("Float array multiplication took %i cycles\n", mulTime);
  }
}