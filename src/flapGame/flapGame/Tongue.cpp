#include <flapGame/Core.h>
#include <flapGame/Tongue.h>
#include <flapGame/Assets.h>

namespace flap {

Tongue::Tongue() {
    const Assets* a = Assets::instance;
    for (u32 i = 0; i < a->bad.tongueBones.numItems(); i++) {
        this->states[0].pts.append(Float3{0.4f * i, 0, 0});
    }
    this->states[1] = this->states[0];
}

void Tongue::update(const Quaternion& birdToWorldRot, float dt) {
    const Assets* a = Assets::instance;
    s32 iters = 1;
    float gravity = 0.001f;
    for (; iters > 0; iters--) {
        this->curIndex = 1 - this->curIndex;
        auto& curState = this->states[this->curIndex];
        auto& prevState = this->states[1 - this->curIndex];
        PLY_ASSERT(curState.pts.numItems() == prevState.pts.numItems());

        // Set first particles
        curState.rootRot = birdToWorldRot * a->bad.tongueRootRot;
        curState.pts[0] = birdToWorldRot * a->bad.tongueBones[0].midPoint;
        Float3 fwd = curState.rootRot.rotateUnitY();

        for (u32 i = 1; i < curState.pts.numItems(); i++) {
            Float3 step = prevState.pts[i] - curState.pts[i];
            step *= 0.98f;
            curState.pts[i] = prevState.pts[i] + step;
            curState.pts[i].z -= gravity;
        }

        // Constrain second point to cone around the mouth
        {
            float angle = 45.f * Pi / 180.f;
            Float2 coneCS = {cosf(angle), sinf(angle)};
            Float3 ray = curState.pts[1] - curState.pts[0];
            float d = dot(ray, fwd);
            Float3 perp = ray - fwd * d;
            float pL = perp.length();
            if (d < 0.1f) {
                if (pL < 1e-4f) {
                    Float3 notCollinear = (fabsf(fwd.x) < 0.9f) ? Float3{1, 0, 0} : Float3{0, 1, 0};
                    perp = cross(fwd, notCollinear);
                    pL = 1.f;
                }
                curState.pts[1] = curState.pts[0] + fwd * coneCS.x + perp * (coneCS.y / pL);
            } else {
                if (pL > 1e-4f) {
                    Float3 norm = -fwd * coneCS.y + perp * (coneCS.x / pL);
                    PLY_ASSERT(norm.isUnit());
                    float nd = dot(norm, ray);
                    if (nd > 0) {
                        curState.pts[1] -= norm * nd;
                    }
                }
            }
        }

        // Constraints
        for (u32 i = 1; i < curState.pts.numItems(); i++) {
            Float3* part = &curState.pts[i];

            // segment length
            float segLen = (a->bad.tongueBones[i - 1].length + a->bad.tongueBones[i].length) * 0.5f;

            // FIXME: div by zero
            Float3 dir = (part[0] - part[-1]).normalized();
            part[0] = part[-1] + dir * segLen;
        }
    }
}

} // namespace flap
