import time
import pyvista as pv
import serial

def get_xyz():

pointa = [1.0, 0.0, 0.0]
pointb = [1.0, 1.0, 0.0]
pointc = [0.0, 1.0, 0.0]
rectangle = pv.Rectangle([pointa, pointb, pointc])

transform_post = (
    pv.Transform().rotate_x(0)
    * pv.Transform().rotate_y(0)
    * pv.Transform().rotate_z(0)
)

pl = pv.Plotter()
actor = pl.add_mesh(rectangle, color='goldenrod')
pl.add_axes_at_origin(labels_off=True)
pl.show(interactive_update=True, auto_close=False)

step_x = pv.Transform().rotate_x(0)
step_y = pv.Transform().rotate_y(0)
step_z = pv.Transform().rotate_z(0)

while pl.render_window is not None:
    transform_post = transform_post * step_x * step_y * step_z
    actor.user_matrix = transform_post.matrix
    pl.update()
    time.sleep(1 / 120)

pl.add_timer_event(max_steps=200_000, duration=16, callback=rotate)
pl.close()
