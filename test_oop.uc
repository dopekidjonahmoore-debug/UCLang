class Animal {
    name == "".
    age == 0.

    def init(name, age.)
        this.name == name.
        this.age == age.
    )

    def speak().
        print("Animal speaks\n").
    )
}

class Dog extends Animal {
    breed == "".

    def init(name, age, breed.)
        this.name == name.
        this.age == age.
        this.breed == breed.
    )

    def speak().
        print("Woof! I am ").
        print(this.name).
        print("\n").
    )

    def getAge().
        response this.age.
    )
}

dog == new Dog("Buddy", 3, "Golden").
dog.speak().
age == dog.getAge().
print("Age: ").
print(age).
print("\n").
