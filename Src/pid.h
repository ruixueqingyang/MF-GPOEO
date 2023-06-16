/*******************************************************************************
Copyright(C), 2022-2022, 瑞雪轻飏
     FileName: pid.h
       Author: 瑞雪轻飏
      Version: 0.01
Creation Date: 20220614
  Description: 1. PID 控制器 头文件, 定义 PID 类
               2.
       Others: //其他内容说明
*******************************************************************************/

#ifndef __PID_H
#define __PID_H

#include <math.h>
#include "global_var.h"
#include "measure.h"
#include "analysis.h"

enum CTRL_CONFIG_TYPE{ Power, Clk }; // 控制参数类型: 功率上限 / 频率上限

class PID_CTRL{
public:
    CTRL_CONFIG_TYPE ctrl_type;
    int count; // 调整次数计数
    // int controlStatus;   //状态，0是未进入运行稳定阶段，1是调节内存频率阶段，2是调节功率上限阶段
    double PerfLossConstraint;       // 性能损失约束
    double actual;       // 实际值
    double err;          // 偏差值
    double prev_err;     // 上一个偏差值
    double Kp, Ki, Kd;   // 比例、积分、微分系数
    double currKp, currKi, currKd;
    int control_var;              // 控制量: GPU频率档位上限
    int prev_control_var;         // 上次的控制量: 上一次调节后的GPU功率上限
    int OptVar;
    int var_max;
    int var_min; // 控制量的最大最小范围
    double integral;     // 积分值
    double differential; // 微分值
    double delta;       // PID控制器输出
    bool isDoubling;

    double minObj, leftObj, rightObj; // 目标函数 最小值
    int opt_control_var, left_control_var, right_control_var; // 取得 目标函数 最小值 时 的 控制量/档位

    std::vector<int> vecSMGear; // SM频率档位历史记录
    std::vector<double> vecActualPerfLoss; // 实际性能损失历史记录
    std::vector<double> vecErr; // 误差历史记录 
    std::vector<double> vecKp; // Kp 历史记录
    std::vector<double> vecActualSMClk; // 实际SM频率历史记录

    // PID 控制
    int control(double in_actual, double ActualObj, float ActualSMClk) { // 实际性能误差, 实际核心平均频率
        actual = in_actual;
        // err = actual - PerfLossConstraint;
        err = PerfLossConstraint - actual;
        integral += err;
        differential = -1 * (err - prev_err);

        vecActualPerfLoss[count%vecActualPerfLoss.size()] = actual;
        vecErr[count%vecErr.size()] = err;
        vecActualSMClk[count%vecActualSMClk.size()] = ActualSMClk;
        count++;

        if (count >= 2) {
            if (vecErr[(count-2)%vecErr.size()] * vecErr[(count-1)%vecErr.size()] < 0) {
                // 若误差正负号改变, 则重置积分项, 从而保证积分项对于最近累计/稳态误差的快速反应/响应
                integral = err;
                currKi = 0.0;
            } else {
                currKi = Ki;
            }
        } else {
            currKi = 0.0;
        }
        
        double smallSlope = -1.0 * (PerfLossConstraint/(vecSMClk.size()-1)) * 3;
        if (count >= 2) { // 已经至少进行了1次控制, 要进行下次控制时
            int gear_1 = vecSMGear[(count-1)%vecSMGear.size()];
            int gear_2 = vecSMGear[(count-2)%vecSMGear.size()];

            double ActualSMClk_1 = vecActualSMClk[(count-1)%vecActualSMClk.size()];
            double ActualSMClk_2 = vecActualSMClk[(count-2)%vecActualSMClk.size()];

            double state_1 = vecActualPerfLoss[(count-1)%vecActualPerfLoss.size()];
            double state_2 = vecActualPerfLoss[(count-2)%vecActualPerfLoss.size()];

            double delta_1 = state_1 - state_2;
            double slope_1, slope_2;
            if (gear_1 == gear_2) {
                slope_1 = smallSlope;
            } else {
                slope_1 = (state_1-state_2) / (gear_1-gear_2);
            }

            bool isSMClkEqual = true;
            bool isMonotonous = true;

            if (count >= 3)
            {
                for (int i = 2; i >= 1; i--)
                {
                    int gear_new = vecSMGear[(count-i)%vecSMGear.size()];
                    int gear_old = vecSMGear[(count-i-1)%vecSMGear.size()];
                    // int state_new = vecActualPerfLoss[(count-i)%vecActualPerfLoss.size()];
                    // int state_old = vecActualPerfLoss[(count-i-1)%vecActualPerfLoss.size()];
                    double ActualSMClk_new = vecActualSMClk[(count-i)%vecActualSMClk.size()];
                    // double ActualSMClk_old = vecActualSMClk[(count-i-1)%vecActualSMClk.size()];
                    MFGPOEOOutStream << "PID: gear_old = " << gear_old << "; gear_new = " << gear_new << std::endl;
                    if (gear_new >= gear_old) { // 判断频率上限不是单调减
                        isMonotonous = false;
                    }
                    if (vecSMClk[gear_new] - ActualSMClk_new > 2) // 判断实际频率!=频率上限
                    {
                        isSMClkEqual = false;
                    }
                }
            } else {
                isSMClkEqual = false;
                isMonotonous = false;
            }
            MFGPOEOOutStream << "PID: isMonotonous = " << isMonotonous << "; isSMClkEqual = " << isSMClkEqual << std::endl;


            MFGPOEOOutStream << "PID: state_2 = " << std::scientific << std::setprecision(3) << 100 * state_2 << " %; state_1 = " << 100 * state_1 << " %" << std::endl;
            MFGPOEOOutStream << "PID: PerfLossConstraint = " << std::scientific << std::setprecision(3) << 100 * PerfLossConstraint << " %" << std::endl;
            
            // 性能损失远远没有达到性能损失阈值
            // 且 实际平均频率小于设定的频率上限 或者 斜率过于平缓
            // 则 增大Kp
            if (state_1 < 0.2 * PerfLossConstraint && state_2 < 0.2 * PerfLossConstraint 
                && ((double)vecSMClk[gear_1] - ActualSMClk_1 > 0.5 || slope_1 > smallSlope)) 
            {
                if (isDoubling == false) {
                    currKp = 1.5 * Kp;
                    isDoubling = true;
                } else {
                    currKp *= 1.5;
                }
                MFGPOEOOutStream << "PID: currKp *= 1.5; currKp = " << std::scientific << std::setprecision(2) << currKp << std::endl;

                currKp = std::min((double)8.0, currKp);

            // 如果多次: 频率上限单调减 且 实际频率==频率上限 且 距离性能损失阈值较远
            } else if (isMonotonous == true && state_1 < 0.5 * PerfLossConstraint) {
                if (isDoubling == false) {
                    currKp = 1.75 * Kp;
                    isDoubling = true;
                } else {
                    currKp *= 1.75;
                }
                MFGPOEOOutStream << "PID: currKp *= 1.75; currKp = " << std::scientific << std::setprecision(2) << currKp << std::endl;

                currKp = std::min((double)8.0, currKp);

            // 当略微超过设定的性能损失阈值时, 则 增大 Kp
            } else if (actual < PerfLossConstraint && PerfLossConstraint - actual < 0.04) {
                currKp = Kp * 0.75;
                isDoubling == false;
                MFGPOEOOutStream << "PID: currKp = Kp * 0.75; currKp = " << std::scientific << std::setprecision(4) << currKp << std::endl;

            } else if (actual < PerfLossConstraint && PerfLossConstraint - actual < 0.1 && std::abs(actual/PerfLossConstraint) > 2) {
                currKp = Kp * 2.2;
                isDoubling == false;
                MFGPOEOOutStream << "PID: currKp = Kp * 2.2; currKp = " << std::scientific << std::setprecision(4) << currKp << std::endl;

            } else if (actual < PerfLossConstraint && PerfLossConstraint - actual < 0.4 && std::abs(actual/PerfLossConstraint) > 3) {
                currKp = Kp * 2.5;
                isDoubling == false;
                MFGPOEOOutStream << "PID: currKp = Kp * 2.5; currKp = " << std::scientific << std::setprecision(4) << currKp << std::endl;

            // 当略微超过设定的性能损失阈值时, 则 增大 Kp
            } else if (actual > PerfLossConstraint && actual - PerfLossConstraint < 0.003) {
                currKp = Kp * 4;
                isDoubling == false;
                MFGPOEOOutStream << "PID: currKp = Kp * 4; currKp = " << std::scientific << std::setprecision(4) << currKp << std::endl;

            // 当略微超过设定的性能损失阈值时, 则 增大 Kp
            } else if (actual > PerfLossConstraint && actual - PerfLossConstraint < 0.006) {
                currKp = Kp * 2.5;
                isDoubling == false;
                MFGPOEOOutStream << "PID: currKp = Kp * 2.5; currKp = " << std::scientific << std::setprecision(4) << currKp << std::endl;

            // 当略微超过设定的性能损失阈值时, 则 增大 Kp
            } else if (actual > PerfLossConstraint && actual - PerfLossConstraint < 0.025) {
                currKp = Kp * 1.75;
                isDoubling == false;
                MFGPOEOOutStream << "PID: currKp = Kp * 1.75; currKp = " << std::scientific << std::setprecision(4) << currKp << std::endl;

            // 重置 currKp
            } else {
                currKp = Kp;
                isDoubling == false;
                MFGPOEOOutStream << "PID: currKp = Kp; currKp = " << std::scientific << std::setprecision(4) << currKp << std::endl;
            }
        } else {
            currKp = 0.7 * Kp;
            isDoubling = false;
        }

        if (count >= 2)
        { // 认为出现振荡, 引入 Kd, 抑制振荡
            MFGPOEOOutStream << "PID: differential = " << std::scientific << std::setprecision(4) << differential << std::endl;
            MFGPOEOOutStream << "PID: vecErr[-1] = " << std::scientific << std::setprecision(4) << vecErr[(count-1)%vecErr.size()] << std::endl;
            MFGPOEOOutStream << "PID: PerfLossConstraint = " << std::scientific << std::setprecision(4) << PerfLossConstraint << std::endl;
            if (differential * vecErr[(count-1)%vecErr.size()] < 0 // err 和 differential 异号
                && std::abs(differential / PerfLossConstraint) > 0.5) // |differential| 较大
                // && std::abs(vecErr[(count-1)%vecErr.size()] / PerfLossConstraint) > 0.5) // |err| 较大
            {
                currKd = Kd;
            } else
            {
                currKd = 0.0;
            }
        } else
        {
            currKd = 0.0;
        }
        
        MFGPOEOOutStream << "PID: currKi = " << std::scientific << std::setprecision(4) << currKi << std::endl;
        MFGPOEOOutStream << "PID: currKd = " << std::scientific << std::setprecision(4) << currKd << std::endl;
        
        // delta = Kp * err + Ki * integral + Kd * differential;
        delta = currKp * err + currKi * integral + currKd * differential;
        prev_err = err;

        std::cout << "control: PerfLossConstraint: " << std::fixed << std::setprecision(4) << PerfLossConstraint << std::endl;
        std::cout << "control: actual: " << std::fixed << std::setprecision(4) << actual << std::endl;
        std::cout << "control: delta: " << std::fixed << std::setprecision(4) << delta << std::endl;
        std::cout << "control: prev_control_var: " << std::dec << prev_control_var << std::endl;

        double tmp_control_var = (double)prev_control_var * (1 - delta);
        control_var = round(tmp_control_var);
        
        std::cout << "control: control_var: " << std::dec << control_var << std::endl;

        // 下边使用一些启发式规则避免一些不稳定/异常情况

        // 如果超过性能阈值, 且两次控制给出的频率上限相同, 则 给频率上限 加 1个频率档位
        if (actual > PerfLossConstraint && control_var < var_max && control_var <= prev_control_var) 
        {
            // control_var = std::max(control_var, prev_control_var);
            // control_var += 1;
            control_var = prev_control_var + 1;
            MFGPOEOOutStream << "PID: 核心频率档位增加 1 到 " << std::dec << control_var << std::endl;

        } else if (actual < PerfLossConstraint && control_var > 0 && control_var >= prev_control_var) {
            // prev_control_var -= 1;
            control_var = prev_control_var - 1;
            MFGPOEOOutStream << "PID: 核心频率档位减小 1 到 " << std::dec << control_var << std::endl;

        }

        // 当 降频 反而 性能提升很多 时, 暂停一次 PID 控制, 直接使用上次的 频率档位
        if (count >= 2) {
            int gear = vecSMGear[(count-1)%vecSMGear.size()];
            int prev_gear = vecSMGear[(count-2)%vecSMGear.size()];
            double perf_loss = vecActualPerfLoss[(count-1)%vecSMGear.size()];
            double perv_perf_loss = vecActualPerfLoss[(count-2)%vecSMGear.size()];
            
            if (0.0 < err && err < 0.01 * PerfLossConstraint) {
                control_var = gear;
                MFGPOEOOutStream << "PID: 进入设定的死区, 使用上次档位" << std::endl;
            }
        }
        // 频率档位必须在允许的档位范围内
        control_var = std::max(control_var, var_min);
        control_var = std::min(control_var, var_max);

        // // 保证优化目标最小
        // double prevObj = ActualObj;
        // if (actual > PerfLossConstraint)
        // {
        //     prevObj += 1;
        // }
        // if (actual > PerfLossConstraint && prev_control_var >= opt_control_var)
        // {
        //     left_control_var = prev_control_var;
        //     leftObj = prevObj;
        //     if (prev_control_var > right_control_var)
        //     {
        //         right_control_var = var_max + 1;
        //         rightObj = 1e8;
        //     }
        //     opt_control_var = -1;
        //     minObj = 1e8;
            
        // } else if (left_control_var < 0)
        // {
        //     if (opt_control_var < 0 || opt_control_var == prev_control_var)
        //     {
        //         opt_control_var = prev_control_var;
        //         minObj = prevObj;
        //     } else if (prevObj < minObj)
        //     {   
        //         if (opt_control_var < prev_control_var)
        //         {
        //             left_control_var = opt_control_var;
        //             leftObj = minObj;
        //             opt_control_var = prev_control_var;
        //             minObj = prevObj;
        //         } else if (prev_control_var < opt_control_var)
        //         {
        //             right_control_var = opt_control_var;
        //             rightObj = minObj;
        //             opt_control_var = prev_control_var;
        //             minObj = prevObj;
        //         }
        //     } else if (prevObj > minObj)
        //     {
        //         if (left_control_var < prev_control_var && prev_control_var < opt_control_var)
        //         {
        //             left_control_var = prev_control_var;
        //             leftObj = prevObj;
        //         } else if (opt_control_var < prev_control_var && prev_control_var < right_control_var)
        //         {
        //             right_control_var = prev_control_var;
        //             rightObj = prevObj;
        //         }
        //     }
        // } else if (opt_control_var < 0) {
        //     if (actual > PerfLossConstraint)
        //     {
        //         left_control_var = prev_control_var;
        //         leftObj = prevObj;
        //     } else
        //     {
        //         opt_control_var = prev_control_var;
        //         minObj = prevObj;
        //     }
        // }
        // if (left_control_var >= 0 && opt_control_var > 0)
        // {
        //     if (prev_control_var == left_control_var 
        //         || (prev_control_var == opt_control_var && minObj == prevObj))
        //     {
        //         if (opt_control_var >= (left_control_var+right_control_var)/2)
        //         {
        //             control_var = std::round(left_control_var + 0.618*(opt_control_var-left_control_var));
        //         } else
        //         {
        //             control_var = std::round(opt_control_var + 0.382*(right_control_var-opt_control_var));
        //         }
        //     } else if (prev_control_var == opt_control_var && minObj != prevObj)
        //     {
        //         control_var = prev_control_var;
        //     } else if (opt_control_var < prev_control_var && minObj <= prevObj)
        //     { // GearLow高 GearGolden低 currGear中  GearHigh高
        //         if (prev_control_var < right_control_var)
        //         {
        //             right_control_var = prev_control_var;
        //             rightObj = prevObj;
        //         }
        //         control_var = std::round(left_control_var + 0.618*(opt_control_var-left_control_var));

        //     } else if (opt_control_var < prev_control_var && minObj > prevObj)
        //     { // GearLow高 GearGolden中 currGear低  GearHigh高
        //         if (prev_control_var < right_control_var)
        //         {
        //             left_control_var = opt_control_var;
        //             leftObj = minObj;
        //             opt_control_var = prev_control_var;
        //             minObj = prevObj;
        //         }
        //         control_var = std::round(opt_control_var + 0.382*(right_control_var-opt_control_var));

        //     } else if (prev_control_var < opt_control_var && prevObj <= minObj)
        //     { // GearLow高 currGear低 GearGolden中 GearHigh高
        //         if (left_control_var < prev_control_var)
        //         {
        //             right_control_var = opt_control_var;
        //             rightObj = minObj;
        //             opt_control_var = prev_control_var;
        //             minObj = prevObj;
        //         }
        //         control_var = std::round(left_control_var + 0.618*(opt_control_var-left_control_var));

        //     } else if (prev_control_var < opt_control_var && prevObj > minObj)
        //     { // GearLow高 currGear中 GearGolden低 GearHigh高
        //         if (left_control_var < prev_control_var)
        //         {
        //             left_control_var = prev_control_var;
        //             leftObj = prevObj;
        //         }
        //         control_var = std::round(opt_control_var + 0.382*(right_control_var-opt_control_var));
        //     }
            
        // } else if (opt_control_var <= 0)
        // {
        //     control_var = std::round(left_control_var + 0.618*(right_control_var-left_control_var));
        // }
        
        
        // 频率档位必须在允许的档位范围内
        control_var = std::max(control_var, var_min);
        control_var = std::min(control_var, var_max);

        prev_control_var = control_var;
        vecSMGear[count%vecSMGear.size()] = control_var;
        vecKp[count%vecKp.size()] = currKp;

        // MFGPOEOOutStream  << std::fixed << std::setprecision(3) << "Low = " << left_control_var << "; "
        // << "Golden = " << opt_control_var << "; " << "High = " << right_control_var << std::endl;

        // MFGPOEOOutStream << std::fixed << std::setprecision(3) << "ObjLow = " << leftObj << "; "
        // << "ObjGolden = " << minObj << "; " << "ObjHigh = " << rightObj << std::endl;

        return control_var;
    }

    // 返回 与 PerfLossConstraint 相比的 百分比误差
    double pct_err() {
        return std::abs(err / PerfLossConstraint);
    }

    void set_objective(double in_PerfLossConstraint, int in_control_var) {
        count = 0;
        PerfLossConstraint = in_PerfLossConstraint;
        prev_err = 0.0;
        prev_control_var = in_control_var;
        integral = 0.0;

        minObj = 1e8; // 目标函数 最小值
        opt_control_var = in_control_var; // 取得 目标函数 最小值 时 的 控制量/档位
        OptVar = var_max;
        leftObj = 1e9;
        rightObj = 1e9;
        left_control_var = var_min-1;
        right_control_var = var_max+1;

        vecSMGear[0] = in_control_var;
        vecErr[0] = 0.0;
    }

    void update_objective(double in_PerfLossConstraint) {
        PerfLossConstraint = in_PerfLossConstraint;
        minObj = 1e8; // 目标函数 最小值
        opt_control_var = -1; // 取得 目标函数 最小值 时 的 控制量/档位
        OptVar = var_max;
        leftObj = 1e9;
        rightObj = 1e9;
        left_control_var = var_min-1;
        right_control_var = var_max+1;
    }

    void reset(int in_control_var) {
        count = 0;
        prev_err = 0.0;
        prev_control_var = in_control_var;
        integral = 0.0;

        minObj = 1e8; // 目标函数 最小值
        opt_control_var = in_control_var; // 取得 目标函数 最小值 时 的 控制量/档位
        OptVar = var_max;
        leftObj = 1e9;
        rightObj = 1e9;
        left_control_var = var_min-1;
        right_control_var = var_max+1;

        vecSMGear[0] = in_control_var;
    }

    void init(CTRL_CONFIG_TYPE in_type, double in_Kp, double in_Ki, double in_Kd, int in_var_min, int in_var_max, int history_len) {
        ctrl_type = in_type;
        count = 0;
        Kp = in_Kp;
        Ki = in_Ki;
        Kd = in_Kd;
        currKp = Kp;
        currKi = Ki;
        var_min = in_var_min;
        var_max = in_var_max;
        prev_err = 0.0;
        prev_control_var = 0;
        integral = 0.0;
        PerfLossConstraint = -1.0;
        isDoubling = false;

        minObj = 1e8; // 目标函数 最小值
        leftObj = 1e9;
        rightObj = 1e9;
        opt_control_var = -1; // 取得 目标函数 最小值 时 的 控制量/档位
        OptVar = var_max;
        left_control_var = var_min-1;
        right_control_var = var_max+1;

        vecSMGear.clear();
        vecActualSMClk.clear();
        vecActualPerfLoss.clear();
        vecErr.clear();
        vecKp.clear();
        vecSMGear.resize(history_len);
        vecActualSMClk.resize(history_len);
        vecActualPerfLoss.resize(history_len);
        vecErr.resize(history_len);
        vecKp.resize(history_len);
    }

    PID_CTRL(CTRL_CONFIG_TYPE in_type, double in_Kp, double in_Ki, double in_Kd, int in_var_min, int in_var_max, int history_len) {
        init(in_type, in_Kp, in_Ki, in_Kd, in_var_min, in_var_max, history_len);
    }

    PID_CTRL() {
        init(CTRL_CONFIG_TYPE::Clk, 0, 0, 0, -1, 0x7FFFFFFF, 8);
    }
};


#endif