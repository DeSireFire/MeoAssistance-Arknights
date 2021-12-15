#include "InfrastMfgTask.h"

#include "Controller.h"
#include "MatchImageAnalyzer.h"
#include "Resource.h"

const std::string asst::InfrastMfgTask::FacilityName = "Mfg";

bool asst::InfrastMfgTask::_run()
{
    json::value task_start_json = json::object{
        { "task_type", "InfrastMfgTask" },
        { "task_chain", m_task_chain }
    };
    m_callback(AsstMsg::TaskStart, task_start_json, m_callback_arg);

    set_facility(FacilityName);
    m_all_available_opers.clear();

    swipe_to_the_left_of_main_ui();
    enter_facility(FacilityName, 0);
    click_bottomleft_tab();

    if (!shift_facility_list()) {
        return false;
    }

    return true;
}
