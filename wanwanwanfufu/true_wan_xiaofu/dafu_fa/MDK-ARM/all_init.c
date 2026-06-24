#include "all_init.h"

void pid_init(pid_typedef*pid,float kp,float ki,float kd,float max_out,float max_iout)
{
    if(pid==NULL) 
    return;

    pid->kp=kp;
    pid->ki=ki;
    pid->kd=kd;
    pid->max_out=max_out;
    pid->max_iout=max_iout;

    pid->set=0.0f;
    pid->fdb=0.0f;
    pid->err[0]=0.0f;
    pid->err[1]=0.0f;
    pid->err[2]=0.0f;

    pid->p_out=0.0f;
    pid->i_out=0.0f;
    pid->d_out=0.0f;
    pid->output=0.0f;

}

//pid计算函数
float pid_calc(pid_typedef*pid,float set,float fdb,int max_angle)
{
   
    if(pid== NULL)
        return 0;

    //计算误差
    pid->err[2]=pid->err[1];
    pid->err[1]=pid->err[0];
    pid->err[0]=set - fdb;
 
    
    pid->set=set;
    pid->fdb=fdb;
    
    //计算P、I、D输出
    pid->p_out=pid->kp*pid->err[0];
    pid->i_out+=pid->ki*pid->err[0];
    if(pid->i_out>pid->max_iout)
        pid->i_out=pid->max_iout;
    else if(pid->i_out<-pid->max_iout)
        pid->i_out=-pid->max_iout;
    
    pid->d_out=pid->kd*(pid->err[0]-pid->err[1]);
    
    //计算总输出
    pid->output=pid->p_out+pid->i_out+pid->d_out;
    if(pid->output>pid->max_out)
        pid->output=pid->max_out;
    else if(pid->output<-pid->max_out)
        pid->output=-pid->max_out;
    
    return pid->output;
}