import base64
from dataclasses import dataclass

import f3d
from f3d_ipython import f3d_image_repr_png

IMAGE_COMPARE_JS = """
function init_imagecompare(_id, w, h){
    const parent = document.getElementById(_id);
    const a = parent.querySelector("#a");
    const b = parent.querySelector("#b");

    a.style.position = "absolute";
    b.style.position = "absolute";
    a.classList.add("hover");
    b.classList.add("hover");

    b.style.overflow = "hidden";
    b.style.display = "block";
    b.style.width = (w / 2)+"px";
    b.style.height = h+"px";
    b.classList.add("border");

    parent.addEventListener("mousemove", function trackLocation(e) {
        const position = e.pageX - b.getBoundingClientRect().left;
        b.style.width = Math.min(Math.max(position, 0), w) + "px";
    }, false);
}
"""


@dataclass
class ImageCompare:
    img1: f3d.Image
    img2: f3d.image

    def _repr_html_(self):
        w, h = self.img1.width, self.img1.height
        img_css = f"width:{w}px; height:{h}px; max-width: {w}px;"
        element_id = id(self)
        css = """
        .imagecompare .border {
          border-right:2px solid white;
        }
        .imagecompare .hover:hover {
          cursor: col-resize;
        }
        """

        return f"""
        <style>{css}</style>
        <div class="imagecompare" id="{element_id}" style="{img_css}">
          <span id="a"><img src="{f3d_image_b64(self.img1)}" style="{img_css}"/></span>
          <span id="b"><img src="{f3d_image_b64(self.img2)}" style="{img_css}"/></span>
        </div>
        <script>{IMAGE_COMPARE_JS}</script>
        <script>init_imagecompare("{element_id}", {w}, {h});</script>
        """


def f3d_image_b64(f3d_img: f3d.Image):
    image_str = base64.b64encode(f3d_image_repr_png(f3d_img)).decode("utf-8")
    return f"data:image/png;base64,{image_str}"
