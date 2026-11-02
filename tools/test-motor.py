"""Compile the real motor app/CAN adapter with host HAL stubs and old-source references."""
from pathlib import Path
import subprocess
import shutil
import os
root=Path(__file__).resolve().parents[1]
board=root/'wanwanwanfufu/dbus_switch_energy/fu_motor'
out=root/'build/motor-host-tests'
out.mkdir(parents=True,exist_ok=True)
gcc=shutil.which('gcc')
if not gcc: raise SystemExit('Host gcc is required (MSYS2 UCRT64 on Windows).')
def relative(path): return os.path.relpath(path,out)
base=[gcc,'-std=c11','-O0','-Wall','-Wextra','-Werror',*['-I'+relative(p) for p in [root/'wanwanwanfufu/common',board/'tests/stubs',board/'user/APP',board/'user/BSP']]]
if os.environ.get('HOST_GCC_PREFIX'): base+=['-B',os.environ['HOST_GCC_PREFIX']]
objects=[]
for index,source in enumerate(['user/APP/motor_control.c','user/APP/motor_board.c','user/APP/motor_remote.c','user/BSP/motor_can.c','user/BSP/motor_uart.c','tests/test_motor.c']):
    obj=f'app{index}.o';subprocess.run(base+['-c',relative(board/source),'-o',obj],cwd=out,check=True);objects.append(obj)
obj='motor_link.o'
subprocess.run(base+['-c',relative(root/'wanwanwanfufu/common/motor_link.c'),'-o',obj],cwd=out,check=True);objects.append(obj)
names=['kp_test','ki_test','kd_test','motor1_speed_pid','motor1_target_speed','motor1_pid_out',
       'TIM_Task_Init','TIM_Task_10ms','TIM_Task_ResetSineParams','VOFA_Task_100ms','HAL_TIM_PeriodElapsedCallback']
for mode in ['big','small']:
    obj=f'legacy_{mode}.o'
    aliases=[f'-D{name}={mode.title()}_{name}' for name in names]
    subprocess.run(base+aliases+['-Wno-unused-parameter','-c',relative(board/f'tests/legacy_{mode}/tim_task.c'),'-o',obj],cwd=out,check=True)
    objects.append(obj)
obj='legacy_pid.o'
subprocess.run(base+['-Wno-unused-parameter','-c',relative(board/'tests/legacy_big/all_init.c'),'-o',obj],cwd=out,check=True);objects.append(obj)
exe=out/'test_motor.exe'
subprocess.run(base+objects+['-lm','-o',exe.name],cwd=out,check=True)
subprocess.run([str(exe)],check=True)
