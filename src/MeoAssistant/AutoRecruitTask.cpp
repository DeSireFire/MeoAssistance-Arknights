#include "AutoRecruitTask.h"

#include "Resource.h"
#include "OcrImageAnalyzer.h"
#include "Controller.h"
#include "RecruitImageAnalyzer.h"
#include "ProcessTask.h"
#include "RecruitTask.h"

void asst::AutoRecruitTask::set_select_level(std::vector<int> select_level) noexcept
{
    m_select_level = std::move(select_level);
}

void asst::AutoRecruitTask::set_confirm_level(std::vector<int> confirm_level) noexcept
{
    m_confirm_level = std::move(confirm_level);
}

void asst::AutoRecruitTask::set_need_refresh(bool need_refresh) noexcept
{
    m_need_refresh = need_refresh;
}

void asst::AutoRecruitTask::set_max_times(int max_times) noexcept
{
    m_max_times = max_times;
}

void asst::AutoRecruitTask::set_use_expedited(bool use_or_not) noexcept
{
    m_use_expedited = use_or_not;
}

bool asst::AutoRecruitTask::_run()
{
    json::value task_start_json = json::object{
        { "task_type", "RecruitTask" },
        { "task_chain", m_task_chain },
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    if (!check_recruit_home_page()) {
        return false;
    }

    analyze_start_buttons();

    // 不使用加急许可的正常公招
    for (; m_cur_times < m_max_times && m_cur_times < m_start_buttons.size(); ++m_cur_times) {
        if (need_exit()) {
            return false;
        }
        if (!recruit_index(m_cur_times)) {
            return false;
        }
    }
    if (!m_use_expedited) {
        return true;
    }
    // 使用加急许可
    for (; m_cur_times < m_max_times; ++m_cur_times) {
        if (need_exit()) {
            return false;
        }
        if (!recruit_now()) {
            return true;
        }
        if (need_exit()) {
            return false;
        }
        analyze_start_buttons();
        if (!recruit_index(0)) {
            return false;
        }
    }

    return true;
}

bool asst::AutoRecruitTask::analyze_start_buttons()
{
    OcrImageAnalyzer start_analyzer;
    const auto start_task_ptr = std::dynamic_pointer_cast<OcrTaskInfo>(Task.get("StartRecruit"));
    start_analyzer.set_task_info(*start_task_ptr);

    auto image = Ctrler.get_image();
    start_analyzer.set_image(image);
    if (!start_analyzer.analyze()) {
        return false;
    }
    start_analyzer.sort_result();
    m_start_buttons = start_analyzer.get_result();
    return true;
}

bool asst::AutoRecruitTask::recruit_index(size_t index)
{
    int delay = Resrc.cfg().get_options().task_delay;

    if (m_start_buttons.size() <= index) {
        return false;
    }
    Rect button = m_start_buttons.at(index).rect;
    Ctrler.click(button);
    sleep(delay);

    return calc_and_recruit();
}

bool asst::AutoRecruitTask::calc_and_recruit()
{
    RecruitTask recurit_task(m_callback, m_callback_arg);
    recurit_task.set_retry_times(m_retry_times);
    recurit_task.set_param(m_select_level, true);
    recurit_task.set_task_chain(m_task_chain);

    // 识别错误，放弃这个公招位，直接返回
    if (!recurit_task.run()) {
        m_callback(AsstMsg::RecruitError, json::value(), m_callback_arg);
        click_return_button();
        return true;
    }

    int maybe_level = recurit_task.get_maybe_level();
    if (need_exit()) {
        return false;
    }
    // 尝试刷新
    if (m_need_refresh && maybe_level == 3
        && !recurit_task.get_has_special_tag()
        && recurit_task.get_has_refresh()) {
        if (refresh()) {
            return calc_and_recruit();
        }
    }
    if (need_exit()) {
        return false;
    }
    if (std::find(m_confirm_level.cbegin(), m_confirm_level.cend(), maybe_level) != m_confirm_level.cend()) {
        confirm();
    }
    else {
        click_return_button();
    }
    return true;
}

bool asst::AutoRecruitTask::check_recruit_home_page()
{
    ProcessTask task(*this, { "RecruitFlag" });
    return task.run();
}

bool asst::AutoRecruitTask::recruit_now()
{
    ProcessTask task(*this, { "RecruitNow" });
    return task.run();
}

bool asst::AutoRecruitTask::confirm()
{
    ProcessTask confirm_task(*this, { "RecruitConfirm" });
    return confirm_task.run();
}

bool asst::AutoRecruitTask::refresh()
{
    ProcessTask refresh_task(*this, { "RecruitRefresh" });
    return refresh_task.run();
}
