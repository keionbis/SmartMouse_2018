
# coding: utf-8

# # Time Optimal Velocity Profiles
# 
# When the maze solver commands that the robot go forward, it can say that it must go forward one or more squares depending on what it knows about the maze. When we don't know what is after the square we pass through, we must be going slow enough to handle any scenario. In other words, there is some $V_f$ that we must reach by the end of our motion. We also begin motions at this speed, since between we arrived where we are we required that we reach $V_f$ to get there. Therefore, we start and end at $V_f$, and we want to cover some distance $d$ in the fast possible time. To do so, we accelerate at our fixed $a$ until we reach max speed, or until we need to start slowing down (whichever comes first). This gives us a trapezoid shaped velocity profile.

# ## Going Straight

# In[1]:

get_ipython().magic('load_ext tikzmagic')


# In[2]:

get_ipython().run_cell_magic('tikz', '-s 400,400', '\\draw[->] (0,0) -- (10,0);\n\\draw[->] (0,0) -- (0,5);\n\n\\draw[line width=1] (0,0.5) -- (2.5,3);\n\\draw[line width=1] (2.5,3) -- (5.5,3);\n\\draw[line width=1] (5.5,3) -- (8,0.5);\n\\draw[dashed] (0,0.5) -- (10,0.5);\n\\draw[dashed] (0,3) -- (10,3);\n\\draw[dashed] (2.5,0) -- (2.5,5);\n\\draw[dashed] (5.5,0) -- (5.5,5);\n\\draw[dashed] (8,0) -- (8,5);\n\n\\draw (-0.5, 0.5) node {$V_{f}$};\n\\draw (-0.5, 3) node {$V_{max}$};\n\\draw (2.5, -0.5) node {$t_b$};\n\\draw (5.5, -0.5) node {$t_f-t_b$};\n\\draw (8, -0.5) node {$t_f$};')


# The time to accelerate from $V_f$ to $V_{max}$ is $t_b = \frac{V-V_f}{a}$. We can substitute this into newtons first equation of motion as follows.
# 
# \begin{align}
# d &= Vt_b - \frac{1}{2}a{t_b}^2 \\
#   &= V\Big(\frac{V-V_f}{a}\Big) - \frac{1}{2}a\Big(\frac{V-V_f}{a}\Big)^2 \\
#   &= \Big(\frac{V^2-VV_f}{a}\Big) - \Big(\frac{a(V-V_f)^2}{2a^2}\Big) \\
#   &= \Big(\frac{2V^2-2VV_f}{2a}\Big) - \Big(\frac{V^2-2VV_f+{V_f}^2}{2a}\Big) \\
#   &= \frac{2V^2-2VV_f - V^2 + 2VV_f - {V_f}^2}{2a} \\
# d &= \frac{V^2-{V_f}^2}{2a} \\
# \end{align}
# 
# For example, if you're at starting at $V_f=0.2\frac{m}{s}$, and you're ramping up to $V=0.5\frac{m}{s}$, and you're acceleration is fixed at the $a=2\frac{m}{s^2}$, the distance you'll need to do that is $d = \frac{0.5 - 0.2}{2*2} = 0.075m$

# ## Code that proves it

# In[3]:

import numpy as np
import matplotlib.pyplot as plt
np.set_printoptions(suppress=True, precision=3)

def profile(V0, Vf, Vmax, d, A, buffer=3e-3):
    v = V0
    x = 0
    a = A
    vs = [v]
    xs = [x]
    a_s = [a]
    
    dt = 0.01
    while x < d:
        x = x + v*dt + a*dt*dt/2.0
        v = v + a*dt
        ramp_d = (v*v+ - Vf*Vf) / (2.0*A)
        if (d-x) < ramp_d + buffer:
            a = -A
        elif v < Vmax:
            a = A
        else:
            a = 0
        
        if v > Vmax:
            v = Vmax
        elif v < Vf:
            v = Vf
                
        xs.append(x)
        vs.append(v)
        a_s.append(a)
        
    return xs, vs, a_s

def graph(title, idx):
    plt.figure()
    plt.title(title)
    Vs = [0.35, 0.5, 0.75, 1, 2]
    Vf = 0.02
    V0 = 0.2
    d = 0.35
    a = 2
    for V in Vs:    
        results  = profile(V0, Vf, V, d, a)
        vs = results[1]
        if V == 2: # make V=2 dashed so we can see it over V=1
            plt.plot(results[idx], label='V={}'.format(V), linestyle='dashed')
        else:
            plt.plot(results[idx], label='V={}'.format(V))
        plt.legend(bbox_to_anchor=(1, 1), loc=2)

graph("position", 0)
graph("velocity", 1)
graph("acceleration", 2)
plt.show()


# ## Taking Turns

# Were we will discuss how to generate a time optimal trajectory for turns. First, let's start out with a generating trajectories that are not time optimal, but rely on specifying the final time $v_f$. For smartmouse, our state space is $[x, y, \theta]$, and a turn can be defined as starting at a point $[x_0, y_0, \theta_0]$ and going to $[x_f, y_f, \theta_0]$. Of course, we also want to specify the velocities at these point, $[\dot{x}_0, \dot{y}_0,\dot{\theta}_0]$ and $[\dot{x}_f, \dot{y}_f,\dot{\theta}_f]$. We have four constraints, so if we want to fit a smooth polynomial to those points we need a 4th order polynomial.
# 
# $$q(t) = a_0 + a_1t + a_2t^2 + a_3t^3$$
# $$\dot{q}(t) = a_1 + 2a_2t + 3a_3t^2$$
# 
# If we sub in our constraints, we get the following system of equations.
# 
# \begin{align}
# q(0) &= a_0 \\
# \dot{q}(0) &= a_1 \\
# q(t_f) &= a_0 + a_1t_f + a_2{t_f}^2 + a_3{t_f}^3\\
# \dot{q}(t_f) &= a_1 + 2a_2t_f + 3a_3{t_f}^2\\
# \end{align}
# 
# In matrix form that looks like:
# \begin{equation}
# \begin{bmatrix}
# 1 & 0 & 0 & 0 \\
# 0 & 1 & 0 & 0 \\
# 1 & t_f & t_f^2 & t_f^3 \\
# 0 & 1 & 2t_f & 3t_f^2 \\
# \end{bmatrix}
# \begin{bmatrix}
# a_0 \\
# a_1 \\
# a_2 \\
# a_3 \\
# \end{bmatrix} =
# \begin{bmatrix}
# q(0) \\
# \dot{q}(0) \\
# q(t_f) \\
# \dot{q}(t_f) \\
# \end{bmatrix}
# \end{equation}
# 
# It can be shown that the matrix on the left is invertable, so long as $t_f-t_0 > 0$. So we can invert and solve this equation and get all the $a$ coefficients. We can then use this polynomial to generate the $q(t)$ and $\dot{q}(t)$ -- our trajectory.

# In[4]:

# Example: you are a point in space (one dimension) go from rest at the origin to at rest at (0.18, 0, 0) in 1 second
import numpy as np
np.set_printoptions(suppress=True, precision=3)

q_0 = np.array([0])
q_dot_0 = np.array([0])
q_f = np.array([0.18])
q_dot_f = np.array([0])
t_f = 1

b = np.array([q_0, q_dot_0, q_f, q_dot_f])
a = np.array([[1,0,0,0],[0,1,0,0],[1, t_f, pow(t_f,2),pow(t_f,3)],[0,1,2*t_f,3*pow(t_f,2)]])
coeff = np.linalg.solve(a, b)
print(coeff)


# Here you can see that the resulting coeffictions are $a_0=0$, $a_1=0$, $a_2=0.54$, $a_0=-0.36$. Intuitively, this says that we're going to have positive acceleration, but our acceleration is going to slow down over time. Let's graph it!

# In[5]:

import matplotlib.pyplot as plt
dt = 0.01
ts = np.array([[1, t, pow(t,2), pow(t,3)] for t in np.arange(0, t_f+dt,  dt)])
qs = ts@coeff
plt.plot(ts[:,1], qs, label="x")
plt.xlabel("time (seconds)")
plt.xlabel("X (meters)")
plt.legend(bbox_to_anchor=(1,1), loc=2)
plt.show()


# **ooooooooooh so pretty**
# 
# Let's try another example, now with our full state space of $[x, y, \theta]$.

# In[6]:

# In this example, we go from (0.18, 0.09, 0) to (0.27,0.18, -1.5707). Our starting and ending velocities are zero
q_0 = np.array([0.09,0.09,0])
q_dot_0 = np.array([0,0,0])
q_f = np.array([0.27,0.18,-1.5707])
q_dot_f = np.array([0,0,0])
t_f = 1

b = np.array([q_0, q_dot_0, q_f, q_dot_f])
a = np.array([[1,0,0,0],[0,1,0,0],[1, t_f, pow(t_f,2),pow(t_f,3)],[0,1,2*t_f,3*pow(t_f,2)]])
coeff = np.linalg.solve(a, b)
print(coeff)

dt = 0.1
ts = np.array([[1, t, pow(t,2), pow(t,3)] for t in np.arange(0, t_f+dt,  dt)])
qs = ts@coeff

plt.rc('text', usetex=True)
plt.rc('font', family='serif')
plt.gca().set_adjustable("box")
plt.subplot(221)
plt.plot(ts[:,1], qs[:,0])
plt.xlabel("time (seconds)")
plt.title("x")
plt.subplot(222)
plt.plot(ts[:,1], qs[:,1])
plt.xlabel("time (seconds)")
plt.title("y")
plt.subplot(223)
plt.plot(ts[:,1], qs[:,2])
plt.xlabel("time (seconds)")
plt.title(r"$\theta$")
plt.subplot(224)
plt.scatter(qs[:,0], qs[:,1])
plt.axis('equal')
plt.xlabel("X")
plt.ylabel("Y")
plt.tight_layout()
plt.show()

# Gifs!
from matplotlib.animation import FuncAnimation
from IPython.display import HTML
fig, ax = plt.subplots()

x = np.arange(0, 20, 0.1)
line, = ax.plot(x, x - 5, 'r-', linewidth=2)

def update(i):
    label = "t_" + str(i)
    line.set_ydata(x - 5 + i)
    ax.set_xlabel(label)
    return line, ax

plt.rc('text', usetex=False)
anim = FuncAnimation(fig, update, frames=np.arange(0, 10), interval=100)
gif_file = 'car.gif'
anim.save(gif_file, dpi=80, writer='imagemagick')
HTML("<img src={}/>".format(gif_file))


# Well, they are smooth, but these are not possible to execute! The robot cannot simply translate sideways.

# In[ ]:




# In[ ]:




# In[ ]:



