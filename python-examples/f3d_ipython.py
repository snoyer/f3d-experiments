from io import BytesIO

import f3d

try:
    from IPython import get_ipython
    from PIL import Image, ImageOps

    def f3d_to_pil(img: f3d.image) -> Image.Image:
        size = img.width, img.height
        depth = img.channel_count
        im = Image.frombytes("RGB" if depth == 3 else "RGBA", size, img.content)
        return ImageOps.flip(im)

    def f3d_image_repr_png(f3d_img: f3d.image):
        buf = BytesIO()
        f3d_to_pil(f3d_img).save(buf, format="png")
        buf.seek(0)
        return buf.getvalue()

    if ipython := get_ipython():
        png_formatter = ipython.display_formatter.formatters["image/png"]
        png_formatter.for_type(f3d.image, f3d_image_repr_png)
except ImportError:
    pass
