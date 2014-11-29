#ifndef CLOTHES_REPRESENTATION_H
#define CLOTHES_REPRESENTATION_H

typedef int TagId;

enum Gender
{
    MALE = 0,
    FEMALE,
    UNISEX
};

enum ClothesGenus
{
    UPPER_BODY = 0,
    LOWER_BODY,
    ENTIRE_BODY,
    OTHER
};

enum ClothesSpecies
{
    SHIRT = 0,
    T_SHIRT,
    TOP,
    BLOUSE,
    PANTS,
    HALF_PANTS,
    THREE_QUARTERS,
    PYJAMA,
    SKIRT,
    DRESS,
    SAREE,
    NOT_APPLICABLE
};

enum Material
{
    COTTON = 0,
    WOOL,
    SYNTHETIC,
    LYCRA,
    OTHER
};

class Clothes
{
private:
    Gender              sex_;
    ClothesGenus        clothes_genus_;
    ClothesSpecies      clothes_species_;
    Material            material_;
    Season              season_;
    TagId               tag_id_;

public:
    void getGender(Gender &gender);
    void getGenus(ClothesGenus &genus);
    void getSpecies(ClothesSpecies &species);
    void getMaterial(Material &material);
    void getSeason(Season &season);

    void setGender(Gender gender);
    void setGenus(ClothesGenus genus);
    void setSpecies(ClothesSpecies species);
    void setMaterial(Material &material);
    void setSeason(Season &season);
};

#endif // CLOTHES_REPRESENTATION_H
