#include "BeybladeBody.h"

BeybladeBody::BeybladeBody(Layer layer, Disc disc, Driver driver) :
	layerRadius(layer.radius), layerHeight(layer.height), discRadius(disc.radius), discHeight(disc.height),
	driverRadius(driver.radius), driverHeight(driver.height), recoilDistribution(layer.recoilDistribution),
	coefficientOfRestitution(0.8), coefficientOfFriction(0.2),
	mass(layer.mass + disc.mass + driver.mass), velocity(glm::vec3(0.0)), acceleration(glm::vec3(0.0)),
	momentOfInertia(layer.momentOfInertia + disc.momentOfInertia + driver.momentOfInertia)
{    
    // Sum 0.5 * Cd * A for each part
    // Use 0.9 for now using cylindrical approximation
    double linearLayerCA = 0.9 * 2 * layerHeight * layerRadius;
    double linearDiscCA = 0.9 * 2 * discHeight * discRadius;
    double linearDriverCA = 0.9 * driverHeight * driverRadius;
    linearDragTerm = 0.5*(linearLayerCA + linearDiscCA + linearDriverCA);

    // Sum 0.5 * Cd*A * r^2 for each part (rotationalDragCoefficient*layerHeight is C * avg distance extending outwards)
    double angularLayerCAr2 = layer.rotationalDragCoefficient * layerHeight * layerRadius * layerRadius;
    double angularDiscCAr2 = disc.rotationalDragCoefficient * discHeight * discRadius * discRadius;
    double angularDriverCAr2 = driver.rotationalDragCoefficient * driverHeight * driverRadius * driverRadius;
    angularDragTerm = 0.5*(angularLayerCAr2 + angularDiscCAr2 + angularDriverCAr2);
}

BeybladeBody::BeybladeBody() : BeybladeBody(Layer(), Disc(), Driver()) {}

/*--------------------------------------------Specialized Getters--------------------------------------------*/
glm::vec3 BeybladeBody::getNormal() const {
    glm::vec3 normalizedAngularVelocity = glm::normalize(angularVelocity);
    // If the y-component is negative, reverse the vector
    if (normalizedAngularVelocity.y < 0) {
        normalizedAngularVelocity = -normalizedAngularVelocity;
    }
    return normalizedAngularVelocity;
}

glm::vec3 BeybladeBody::getBottomPosition() const {
    glm::vec3 unitDown = angularVelocity.y < 0 ? glm::normalize(angularVelocity) : -glm::normalize(angularVelocity);
    glm::vec3 tiltedDisplacement = dv3(discHeight + driverHeight) * unitDown;
    glm::vec3 bottomPosition = baseCenter + tiltedDisplacement;
    return bottomPosition;
}

void BeybladeBody::setInitialLaunch(glm::vec3 initialCenter, glm::vec3 initialVelocity, glm::vec3 initialAngularVelocity)
{
    baseCenter = initialCenter;
    velocity = initialVelocity;
    angularVelocity = initialAngularVelocity;
}

/*--------------------------------------------Collision Calculations--------------------------------------------*/

double BeybladeBody::sampleRecoil() {
    return recoilDistribution->sample();
}

std::optional<double> BeybladeBody::distanceOverlap(BeybladeBody* a, BeybladeBody* b) {
    if (!a || !b) {
        throw std::invalid_argument("Null pointer created in Beyblade::inContact");
    }

    glm::vec3 aCenter = a->getCenter();
    glm::vec3 bCenter = b->getCenter();

    BeybladeBody* lowerBey = aCenter.y < bCenter.y ? a : b;
    BeybladeBody* higherBey = lowerBey == a ? b : a;

    // Return nothing (no contact) early if layers do not vertically overlap
    if (lowerBey->getCenter().y + lowerBey->getLayerHeight() < higherBey->getCenter().y) {
        return std::nullopt;
    }

    double diffX = aCenter.x - bCenter.x;
    double diffZ = aCenter.z - bCenter.z;
    double squaredDistance = diffX * diffX + diffZ * diffZ;
    double radiiSum = a->getLayerRadius() + b->getLayerRadius();

    double overlapDistance = radiiSum * radiiSum - squaredDistance;

    // Checks for horizontal overlap based on xz coordinates with radii
    if (overlapDistance > 0) {
        return std::optional<double>(sqrt(overlapDistance));
    }
    return std::nullopt;
}

/*--------------------------------------------Accumulators--------------------------------------------*/

void BeybladeBody::accumulateVelocity(glm::vec3 addedVelocity)
{
    accumulatedVelocity += addedVelocity;
}

void BeybladeBody::accumulateAngularVelocity(glm::vec3 addedAngularVelocity)
{
    accumulatedAngularVelocity += addedAngularVelocity;
}

void BeybladeBody::accumulateAcceleration(glm::vec3 addedAcceleration)
{
    //accumulatedAcceleration += addedAcceleration;
    accumulatedAcceleration = addedAcceleration;
}

void BeybladeBody::accumulateAngularAcceleration(glm::vec3 addedAngularAcceleration)
{
    accumulatedAngularAcceleration += addedAngularAcceleration;
}


/*--------------------------------------------Updators--------------------------------------------*/

// Increases or decreases angular speed given angular impulse magnitude
void BeybladeBody::accumulateAngularImpulseMagnitude(double magnitude)
{
    if (glm::length(angularVelocity) < std::numeric_limits<float>::epsilon()) {
        accumulateAngularVelocity(glm::vec3(magnitude, 0.0, 0.0));
        std::cout << "Angular velocity was 0, used default x ins accumulateAngularImpulse\n";
        return;
    }
    glm::vec3 deltaAngularImpulse = dv3(magnitude) * glm::normalize(angularVelocity);
    glm::vec3 deltaAngularVelocity = deltaAngularImpulse / dv3(momentOfInertia);
    accumulateAngularVelocity(deltaAngularVelocity);
}

// Increases or decreases linear speed given linear impulse magnitude
void BeybladeBody::accumulateImpulseMagnitude(double magnitude)
{
    if (glm::length(velocity) < std::numeric_limits<float>::epsilon()) {
        accumulateVelocity(glm::vec3(magnitude, 0.0, 0.0));
        std::cout << "Velocity was 0, used default x ins accumulateImpulse\n";
        return;
    }
    glm::vec3 deltaImpulse = dv3(magnitude) * glm::normalize(velocity);
    glm::vec3 deltaVelocity = deltaImpulse / dv3(mass);
    accumulateVelocity(deltaVelocity);
}

void BeybladeBody::applyAccumulatedChanges(double deltaTime)
{
    printVec3("Accumulated velocity", accumulatedVelocity);
    printVec3("Accumulated angular velocity", accumulatedAngularVelocity);
    printVec3("Accumulated acceleration", accumulatedAcceleration);
    printVec3("Accumulated angular acceleration", accumulatedAngularAcceleration);

    velocity += accumulatedVelocity;
    angularVelocity += accumulatedAngularVelocity;

    velocity += accumulatedAcceleration * dv3(deltaTime);
    angularVelocity += accumulatedAngularAcceleration * dv3(deltaTime);

    accumulatedVelocity = accumulatedAngularVelocity = accumulatedAcceleration = accumulatedAngularAcceleration = glm::vec3(0.0);
}

void BeybladeBody::update(double deltaTime)
{
    baseCenter += velocity * dv3(deltaTime);
    std::cout << "Base center: " << baseCenter.x << ", " << baseCenter.y << ", " << baseCenter.z << std::endl;
}