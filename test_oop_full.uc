# Test inheritance, super, response, method chaining
class Animal {
    name == "".

    def init(name.
        this.name == name.
    )

    def speak(.
        response("...").
    )

    def greet(.
        print("Hello, I am ").
        print(this.name).
        print("\n").
    )
}

class Dog extends Animal {
    breed == "".

    def init(name, breed.
        super.init(name).
        this.breed == breed.
    )

    def speak(.
        response("Woof!").
    )

    def getBreed(.
        response(this.breed).
    )
}

class Cat extends Animal {
    def speak(.
        response("Meow!").
    )
}

a == new Animal("Generic").
a.greet().
print(a.speak()).
print("\n").

d == new Dog("Rex", "Husky").
d.greet().
print(d.speak()).
print("\n").
print(d.getBreed()).
print("\n").

c == new Cat("Whiskers").
c.greet().
print(c.speak()).
print("\n").
