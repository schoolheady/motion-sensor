from time import sleep
import pyvista as pv
import serial
import re
ser = serial.Serial("COM12", "115200", timeout=1)
def get_xyz():
    data = ser.readline()
    str_data = str(data)

    NUM = re.compile(r'\[(-?\d+\.\d+)\]')

    line = "[ACCEL] - [1.345541] [3.960009] [-9.586384] m/s"
    x, y, z = map(float, NUM.findall(str_data))
    #print(x , " - " , y , " - " ,  z)
    return x,y,z
    
# if there is a bracket check if there is a number next, after that if yes, check if there is a bracket to end the value.


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



while pl.render_window is not None:
    x,y,z = get_xyz()
    transform_post = pv.Transform().translate(x, y, z)
    actor.user_matrix = transform_post.matrix
    pl.update()
    sleep(0.0010)

pl.add_timer_event(max_steps=200_000, duration=16, callback=rotate)
pl.close()
