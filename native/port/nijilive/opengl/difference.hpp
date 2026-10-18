#pragma once
#include "resources.hpp"
#include <core/render/common.hpp>
namespace nijiexpose::ogl {
using nicxlive::core::DifferenceEvaluationRegion;
using nicxlive::core::DifferenceEvaluationResult;
void oglSetDifferenceAggregationEnabled(bool);
bool oglIsDifferenceAggregationEnabled();
void oglSetDifferenceAggregationRegion(DifferenceEvaluationRegion);
DifferenceEvaluationRegion oglGetDifferenceAggregationRegion();
bool oglEvaluateDifferenceAggregation(GLuint,int,int);
bool oglFetchDifferenceAggregationResult(DifferenceEvaluationResult&);
}
