from pathlib import Path
root=Path(__file__).resolve().parents[2]
source=(root/'source/nijiexpose/animation/package.d').read_text(encoding='utf-8')
text=source[source.index('    bool playTest'):source.index('\npublic:',source.index('class AnimationControl'))]
text=text.replace('ThresholdDir.','ThresholdDir::').replace('cast(int)','static_cast<int>').replace('cast(bool)','static_cast<bool>').replace('deltaTime()','nicxlive::core::deltaTime()')
(root/'native/port/nijiexpose/animation/thresholds.inc').write_text('// Generated original threshold and hold methods.\n'+text,encoding='utf-8')
