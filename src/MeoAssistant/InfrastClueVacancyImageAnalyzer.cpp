#include "InfrastClueVacancyImageAnalyzer.h"

#include "AsstUtils.hpp"
#include "Logger.hpp"
#include "MatchImageAnalyzer.h"
#include "Resource.h"

bool asst::InfrastClueVacancyImageAnalyzer::analyze()
{
    const static std::string clue_vacancy = "InfrastClueVacancy";

    MatchImageAnalyzer analyzer(m_image);
    for (const std::string& suffix : m_to_be_analyzed) {
        const auto task_ptr = std::dynamic_pointer_cast<MatchTaskInfo>(
            task.get(clue_vacancy + suffix));
        analyzer.set_task_info(*task_ptr);
        if (!analyzer.analyze()) {
            Log.trace("no", clue_vacancy, suffix);
            continue;
        }
        Rect rect = analyzer.get_result().rect;
        Log.trace("has", clue_vacancy, suffix);
#ifdef LOG_TRACE
        cv::rectangle(m_image_draw, utils::make_rect<cv::Rect>(rect), cv::Scalar(0, 0, 255), 2);
        cv::putText(m_image_draw, suffix, cv::Point(rect.x, rect.y + 1), 0, 1, cv::Scalar(0, 0, 255), 2);
#endif
        m_clue_vacancy.emplace(suffix, rect);
    }

    return !m_clue_vacancy.empty();
}