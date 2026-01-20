#include "motion_player.h"
#include <QDebug>
#include <cmath>

MotionPlayer::MotionPlayer(ArmController* armController, QObject* parent)
    : QObject(parent)
    , m_armController(armController)
{
    m_playTimer = new QTimer(this);
    m_playTimer->setSingleShot(true);
    connect(m_playTimer, &QTimer::timeout, this, &MotionPlayer::onTimerTick);
}

void MotionPlayer::play(const Motion& motion) {
    if (motion.isEmpty()) {
        emit errorOccurred("Движение не содержит ключевых кадров");
        return;
    }
    
    if (!m_armController->isConnected()) {
        emit errorOccurred("Робот не подключён");
        return;
    }
    
    // Если уже играем — остановить
    if (m_isPlaying) {
        stop();
    }
    
    m_currentMotion = motion;
    m_currentKeyframe = 0;
    m_loopCount = 0;
    m_isPlaying = true;
    m_isPaused = false;
    
    qDebug() << "Воспроизведение движения:" << motion.name 
             << "кадров:" << motion.keyframeCount()
             << "looping:" << motion.looping;
    
    emit started(motion.name);
    
    // Запускаем первый кадр с плавным переходом
    // (как при loop — вычисляем время по угловому расстоянию)
    executeKeyframeSmooth(0, true);
}

void MotionPlayer::stop() {
    m_playTimer->stop();
    m_isPlaying = false;
    m_isPaused = false;
    m_currentKeyframe = 0;
    m_loopCount = 0;
    
    // Отменяем все запланированные команды на контроллере
    m_armController->cancelAllPendingCommands();
    
    qDebug() << "Воспроизведение остановлено";
    emit stopped();
}

void MotionPlayer::pause() {
    if (m_isPlaying && !m_isPaused) {
        m_playTimer->stop();
        m_isPaused = true;
        qDebug() << "Воспроизведение на паузе";
        emit paused();
    }
}

void MotionPlayer::resume() {
    if (m_isPlaying && m_isPaused) {
        m_isPaused = false;
        qDebug() << "Воспроизведение продолжено";
        emit resumed();
        
        // Продолжаем с текущего кадра
        executeKeyframe(m_currentKeyframe);
    }
}

void MotionPlayer::setSpeed(int percent) {
    m_speed = qBound(25, percent, 400);  // 0.25x - 4x
    qDebug() << "Скорость воспроизведения:" << m_speed << "%";
}

void MotionPlayer::onTimerTick() {
    if (!m_isPlaying || m_isPaused) {
        return;
    }
    
    // ПРОВЕРКА АВАРИЙНОЙ ОСТАНОВКИ
    if (m_armController->isEmergencyStopped()) {
        emit errorOccurred("Аварийная остановка - воспроизведение прекращено");
        stop();
        return;
    }
    
    // Проверяем подключение
    if (!m_armController->isConnected()) {
        emit errorOccurred("Робот отключился во время воспроизведения");
        stop();
        return;
    }
    
    // Проверяем ошибки
    if (m_armController->hasError()) {
        emit errorOccurred("Ошибка робота во время воспроизведения");
        stop();
        return;
    }
    
    // Переходим к следующему кадру
    m_currentKeyframe++;
    
    if (m_currentKeyframe >= m_currentMotion.keyframeCount()) {
        // Конец последовательности
        m_loopCount++;
        emit loopCompleted(m_loopCount);
        
        if (m_currentMotion.looping) {
            // Начинаем сначала
            m_currentKeyframe = 0;
            qDebug() << "Цикл" << m_loopCount << "завершён, начинаем заново";
            
            // ВАЖНО: Плавный переход к начальной позиции
            // Вычисляем расстояние до первого кадра и используем адекватное время
            executeKeyframeSmooth(0, true);  // true = это loop-переход
            return;
        } else {
            // Одноразовое воспроизведение окончено
            qDebug() << "Воспроизведение завершено (не циклическое)";
            stop();
            return;
        }
    }
    
    executeKeyframe(m_currentKeyframe);
}

void MotionPlayer::executeKeyframe(int index) {
    if (index < 0 || index >= m_currentMotion.keyframeCount()) {
        return;
    }
    
    const MotionKeyframe& kf = m_currentMotion.keyframes[index];
    
    emit keyframeChanged(index, m_currentMotion.keyframeCount());
    
    // Вычисляем прогресс в процентах
    int progress = (index * 100) / m_currentMotion.keyframeCount();
    emit progressChanged(progress);
    
    // Время перехода из записи с учётом скорости воспроизведения
    // speed 100% = без изменений
    // speed 200% = в 2 раза быстрее (половина времени)
    // speed 50%  = в 2 раза медленнее (двойное время)
    int transitionMs = adjustedTransitionTime(kf.transitionMs);
    
    // Минимум 300мс для плавности
    transitionMs = qMax(300, transitionMs);
    
    qDebug() << "Кадр" << index << "/" << m_currentMotion.keyframeCount() 
             << "записанное время:" << kf.transitionMs << "мс"
             << "с учётом скорости:" << transitionMs << "мс";
    
    // Отправляем углы напрямую — робот сам сделает плавное движение
    m_armController->setAllJointAngles(kf.jointAngles, transitionMs);
    
    // Планируем следующий кадр через время перехода + небольшой буфер
    int nextTimerMs = transitionMs + 100;
    m_playTimer->start(nextTimerMs);
}

int MotionPlayer::adjustedTransitionTime(int originalMs) const {
    // Скорость 100% = без изменений
    // Скорость 200% = в 2 раза быстрее = половина времени
    // Скорость 50% = в 2 раза медленнее = двойное время
    if (m_speed <= 0) return originalMs;
    
    int adjusted = (originalMs * 100) / m_speed;
    
    // Минимум 300мс для безопасности
    return qMax(300, adjusted);
}

int MotionPlayer::calculateTransitionTime(int targetIndex) const {
    // Вычисляем время перехода на основе максимального углового расстояния
    if (targetIndex < 0 || targetIndex >= m_currentMotion.keyframeCount()) {
        return 1000;  // Дефолт 1 секунда
    }
    
    const MotionKeyframe& targetKf = m_currentMotion.keyframes[targetIndex];
    ArmState currentState = m_armController->getState();
    
    // Находим максимальную дельту угла среди всех суставов
    double maxDelta = 0.0;
    for (int i = 0; i < MOTION_NUM_JOINTS; ++i) {
        if (i == 6) continue;  // Пропускаем грипер
        double delta = std::abs(targetKf.jointAngles[i] - currentState.joints[i].angle);
        if (delta > maxDelta) {
            maxDelta = delta;
        }
    }
    
    // Безопасная скорость: ~30 градусов в секунду = 33мс на градус
    // Для больших перемещений используем медленнее
    int timeMs = static_cast<int>(maxDelta * 33.0);
    
    // Минимум 500мс, максимум 3000мс
    timeMs = qBound(500, timeMs, 3000);
    
    // Применяем настройку скорости
    return adjustedTransitionTime(timeMs);
}

void MotionPlayer::executeKeyframeSmooth(int index, bool isLoopTransition) {
    if (index < 0 || index >= m_currentMotion.keyframeCount()) {
        return;
    }
    
    const MotionKeyframe& kf = m_currentMotion.keyframes[index];
    
    emit keyframeChanged(index, m_currentMotion.keyframeCount());
    emit progressChanged(0);  // Начало нового цикла
    
    // Для loop-перехода вычисляем время на основе расстояния
    int transitionMs;
    if (isLoopTransition) {
        transitionMs = calculateTransitionTime(index);
        qDebug() << "LOOP-переход к кадру 0, вычисленное время:" << transitionMs << "мс";
    } else {
        transitionMs = adjustedTransitionTime(kf.transitionMs);
        transitionMs = qMax(300, transitionMs);
    }
    
    qDebug() << "Кадр" << index << "/" << m_currentMotion.keyframeCount() 
             << "время перехода:" << transitionMs << "мс"
             << (isLoopTransition ? "(ПЛАВНЫЙ LOOP)" : "");
    
    // Используем встроенную интерполяцию робота (одна команда с длительностью)
    // Это обеспечивает более плавное движение, чем ручная отправка координат
    if (isLoopTransition) {
        // Для loop-перехода используем интерполяцию контроллера (она нужна для длинных дистанций)
        // НО исправим её реализацию позже, пока используем обычный метод с большим временем
        // Или вернем как было, если интерполяция работала нормально для loop
        m_armController->setAllJointAnglesInterpolated(kf.jointAngles, transitionMs, 10);
    } else {
        m_armController->setAllJointAngles(kf.jointAngles, transitionMs);
    }
    
    // Планируем следующий кадр с увеличенным запасом
    int nextTimerMs = transitionMs + 150; 
    m_playTimer->start(nextTimerMs);
}
