# mandelbrot_task.py
import math
import time

def render_mandelbrot(
    width=240,
    height=135,
    max_iter=80,
    real_min=-2.0,
    real_max=1.0,
    imag_min=-1.0,
    imag_max=1.0
):
    """
    Render a Mandelbrot fractal and return a 2D array of iteration counts.
    Perfect for distributed compute nodes where each node renders a region.
    """

    start = time.time()

    # Prepare output matrix
    fractal = [[0 for _ in range(width)] for _ in range(height)]

    for y in range(height):
        imag = imag_min + (y / height) * (imag_max - imag_min)
        for x in range(width):
            real = real_min + (x / width) * (real_max - real_min)

            c = complex(real, imag)
            z = 0 + 0j
            iterations = 0

            while abs(z) <= 2 and iterations < max_iter:
                z = z * z + c
                iterations += 1

            fractal[y][x] = iterations

    duration = time.time() - start

    return {
        "width": width,
        "height": height,
        "max_iter": max_iter,
        "render_time_seconds": round(duration, 4),
        "matrix": fractal,
        "summary": f"Rendered fractal {width}x{height} in {duration:.4f}s"
    }


def demo(width, height, max_iter):
    """A short quick version to test on a single node."""
    return render_mandelbrot(width=width, height=height, max_iter=max_iter)