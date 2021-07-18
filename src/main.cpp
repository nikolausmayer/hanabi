/**
 * Author: Nikolaus Mayer, 2021
 */

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
/// AGG (Anti-Grain Geometry)
#include <agg_ellipse.h>
#include <agg_pixfmt_gray.h>
#include <agg_pixfmt_rgb.h>
#include <agg_pixfmt_rgba.h>
#include <agg_alpha_mask_u8.h>
#include <agg_renderer_base.h>
#include <agg_renderer_scanline.h>
#include <agg_scanline_u.h>
#include <agg_scanline_p.h>
#include <agg_rasterizer_scanline_aa.h>
#include <agg_path_storage.h>
#include <agg_conv_curve.h>
#include <agg_conv_dash.h>
#include <agg_conv_stroke.h>
#include <agg_conv_transform.h>
#include <agg_span_image_filter_rgb.h>
#include <agg_span_image_filter_rgba.h>
#include <agg_span_interpolator_linear.h>
#include <agg_renderer_scanline.h>
#include <agg_span_allocator.h>
#include <agg_rendering_buffer.h>
#include <agg_image_accessors.h>
#include <agg_path_storage.h>

#include <FL/Fl.H>
#include <FL/Fl_Image.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>

constexpr int W{512};
constexpr int H{512};

struct Layer {
  agg::rgba8 color;
  float diameter;
  float spacing;
  float angle;
};
Layer layers[] = {
  {{200, 200, 255},  1.000, 6, 0},
  {{255, 255,   0},  1.375, 6, 0},
  {{255,   0,   0},  1.750, 6, 0},
  {{  0,   0, 255},  2.125, 6, 0},
  {{  0,   0,   0},  2.500, 6, 0},
};

unsigned char* imgdata{nullptr};
Fl_RGB_Image* img{nullptr};
Fl_Window* win{nullptr};
Fl_Box* imgbox{nullptr};
Fl_Value_Slider* slider{nullptr};

unsigned char* canvas_buf{nullptr};


void Render()
{
  static agg::rendering_buffer rbuf_alpha;
  static agg::rendering_buffer rbuf_canvas;
  static agg::rasterizer_scanline_aa<> rasterizer;

  // Alpha masks
  static unsigned char* alphamask_buf_outside{nullptr};
  static unsigned char* alphamask_buf_ring{nullptr};
  static unsigned char* alphamask_buf = new unsigned char[W * H];
  // Outer area
  if (not alphamask_buf_outside) {
    alphamask_buf_outside = new unsigned char[W * H];

    rbuf_alpha.attach(alphamask_buf_outside, W, H, W);

    agg::pixfmt_gray8 pixf(rbuf_alpha);
    agg::renderer_base ren_base(pixf);
    agg::renderer_scanline_aa_solid renderer(ren_base);
    agg::scanline_p8 sl;

    ren_base.clear(agg::gray8(0));

    {
      agg::ellipse ell;
      ell.init(256, 256, 200, 200, 100);
      rasterizer.add_path(ell);
      renderer.color(agg::gray8(255));
      agg::render_scanlines(rasterizer, sl, renderer);
    }
  }
  // Outer ring
  if (not alphamask_buf_ring) {
    alphamask_buf_ring = new unsigned char[W * H];
    rbuf_alpha.attach(alphamask_buf_ring, W, H, W);

    agg::pixfmt_gray8 pixf(rbuf_alpha);
    agg::renderer_base ren_base(pixf);
    agg::renderer_scanline_aa_solid renderer(ren_base);
    agg::scanline_p8 sl;

    ren_base.clear(agg::gray8(0));

    {
      agg::ellipse ell;
      ell.init(256, 256, 200, 200, 100);
      rasterizer.add_path(ell);
      renderer.color(agg::gray8(255));
      agg::render_scanlines(rasterizer, sl, renderer);
    }
    {
      rasterizer.reset();
      agg::ellipse ell;
      ell.init(256, 256, 175, 175, 100);
      rasterizer.add_path(ell);
      renderer.color(agg::gray8(0));
      agg::render_scanlines(rasterizer, sl, renderer);
    }
  }

  /// Clear canvas
  {
    rbuf_canvas.attach(canvas_buf, W, H, W*3);
    agg::pixfmt_rgb24 pixf(rbuf_canvas);
    agg::renderer_base ren_base(pixf);
    ren_base.clear(agg::rgba8(255, 255, 255));
  }

  for (const auto& layer : layers) {

    // Hexagonal pattern
    {
      rbuf_alpha.attach(alphamask_buf, W, H, W);

      agg::pixfmt_gray8 pixf(rbuf_alpha);
      agg::renderer_base ren_base(pixf);
      agg::renderer_scanline_aa_solid renderer(ren_base);
      agg::scanline_p8 sl;

      ren_base.clear(agg::gray8(255));

      agg::path_storage paths;
      int yi = 0;
      int xi = 0;
      int direction = 0;
      int streaksize = 1;
      int streak = 0;
      int switches = 0;
      while (true) {
        float xc = xi * layer.spacing + (yi % 2 == 0 ? 0 : 0.5 * layer.spacing);
        float yc = yi * 1.732/2 * layer.spacing;
        if (std::pow(xc, 2) + std::pow(yc, 2) < 200 * 200) {
          agg::ellipse ell;
          ell.init(W/2 + xc, H/2 + yc, layer.diameter, layer.diameter, 32);
          paths.concat_path(ell);
        }

        if (W/2 + xc < 0 or W/2 + xc >= W or
            H/2 + yc < 0 or H/2 + yc >= H)
          break;

        switch (direction) {
          case 0: {--yi; break;}
          case 1: {++xi; break;}
          case 2: {++yi; break;}
          case 3: {--xi; break;}
        }
        ++streak;
        if (streak == streaksize) {
          direction = (direction + 1) % 4;
          streak = 0;
          ++switches;
          if (switches % 2 == 0)
            ++streaksize;
        }
      }

      agg::trans_affine mtx;
      mtx *= agg::trans_affine_translation(-W/2, -H/2);
      mtx *= agg::trans_affine_rotation(layer.angle);
      mtx *= agg::trans_affine_translation(W/2, H/2);
      agg::conv_transform<agg::path_storage, agg::trans_affine> trans(paths, mtx);
      rasterizer.add_path(trans);

      renderer.color(agg::gray8(0));
      agg::render_scanlines(rasterizer, sl, renderer);

      for (std::size_t i = 0; i < W*H; ++i) {
        /// Add outer ring
        alphamask_buf[i] = std::max(alphamask_buf[i], alphamask_buf_ring[i]);
        /// Blank out outer area
        alphamask_buf[i] = std::min(alphamask_buf[i], alphamask_buf_outside[i]);
      }
    }

    // Foreground
    {
      agg::alpha_mask_gray8 alphamask(rbuf_alpha);
      rbuf_canvas.attach(canvas_buf, W, H, W*3);

      agg::pixfmt_rgb24 pixf(rbuf_canvas);
      agg::renderer_base ren_base(pixf);
      agg::renderer_scanline_aa_solid renderer(ren_base);
      agg::scanline_u8_am sl(alphamask);

      agg::ellipse ell;
      ell.init(256, 256, 200, 200, 100);
      rasterizer.add_path(ell);
      renderer.color(layer.color);
      agg::render_scanlines(rasterizer, sl, renderer);
    }

  }  /// <- Layers loop
}


void Update(Fl_Widget *w)
{
  const float angle = slider->value();
  layers[3].angle = angle * M_PI / 180;

  Render();

  img->uncache();
  imgbox->redraw();
}



int main()
{

  canvas_buf = new unsigned char[W * H * 3];

  //{
  //  std::ofstream outfile{"alpha.pgm"};
  //  outfile << "P5\n"
  //          << W << " " << H << "\n"
  //          << 255 << "\n";
  //  outfile.write(reinterpret_cast<const char*>(alphamask_buf), W*H);
  //}
  //{
  //  std::ofstream outfile{"image.ppm"};
  //  outfile << "P6\n"
  //          << W << " " << H << "\n"
  //          << 255 << "\n";
  //  outfile.write(reinterpret_cast<const char*>(canvas_buf), W*H*3);
  //}

  win     = new Fl_Window(W+400,H);

  img     = new Fl_RGB_Image(canvas_buf, W, H, 3);
  imgbox  = new Fl_Box(0,0,W,H);
  imgbox->image(img);
  imgbox->show();

  slider = new Fl_Value_Slider(W,0,400,25);
  slider->type(FL_HOR_NICE_SLIDER);
  slider->bounds(-60, 60);
  slider->value(0);
  slider->callback(Update);
  slider->show();

  win->end();
  win->show();

  Update(0);

  Fl::run();

  delete[] canvas_buf;

  /// Bye!
  return EXIT_SUCCESS;
}

